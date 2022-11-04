
#include <stdio.h>
#include <math.h>
#include "circle.h"

int max_pixelvalue = 255;

float lambda = 0.1;


unsigned short* read_pgm(  int *w , int *h,  const char *fname )
{
	int i;
	int j;
	FILE *f;
	int max;
	int w_,h_;
	int cropw=0;
	int croph=0;
	int k=0;
	int pipe=0;
	char line[256];
	unsigned short *r;
	if( strcmp(fname+strlen(fname)-4,".pgm") != 0 ) {
		char cmd[1024];
		sprintf(cmd,"convert %s pgm:-",fname);
		f = popen(cmd,"r");
		pipe = 1;
	} else 
		f = fopen(fname,"r");
	if(!f) {
		perror("open");
		printf("can't open %s\n",fname );
		return NULL;
	}
	fgets(line,sizeof(line),f);
	do {
		fgets(line,sizeof(line),f);
	} while(line[0] == '#' );
	sscanf(line,"%d %d",&w_,&h_);
	w_ -= 2*cropw;
	h_ -= 2*croph;
	fgets(line,sizeof(line),f);
	sscanf(line,"%d",&max);
	max_pixelvalue = max;
	r = malloc( w_ * h_ * sizeof(short) );
	for(i=0;i<croph*(2*cropw+w_);i++) {
		if( max > 255 )
			fgetc(f);
		fgetc(f);
	}
	for(i=0;i<h_;i++) {
		for(j=0;j<cropw;j++) {
			if( max > 255 )
				fgetc(f);
			fgetc(f);
		}
		for(j=0;j<w_;j++) {
			unsigned char p1 = 0;
			if( max > 255 )
				p1 = fgetc(f);
			unsigned char p2 = fgetc(f);
			r[k++] = ( p1*256 + p2 )  ;
		}
		for(j=0;j<cropw;j++) {
			if( max > 255 )
				fgetc(f);
			fgetc(f);
		}
	}
	if(pipe)
		pclose(f);
	else
		fclose(f);
	*w = w_;
	*h = h_;
	return r;
}

void write_pgm( float *out_img , int w , int h , char *fname , int inv )
{
	FILE *f;
	int x,y;
	f = fopen(fname,"w");
	fprintf(f,"P5\n%d %d\n%d\n",w,h,max_pixelvalue);
	for(y=0;y<h;y+=1)
		for(x=0;x<w;x+=1) {
			int value =  out_img[y*w+x] ;
			if( value < 0 )
				value = 0;
			if( value > max_pixelvalue )
				value = max_pixelvalue;
			if( inv )
				value = max_pixelvalue - value;
			if( max_pixelvalue > 255 )
				fputc( value/256 , f);
			fputc( value&255 , f);
		}
	fclose(f);
}

float weight_[4][4] = {
	{ 1,0,0,0 },
	{ 0,0,0,0},
	{ 0,0,0,0 },
	{ 0,0,0,0 }
};

static int normalized = 0;

static void normalize()
{
	int i,j;
	float sum=0;
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			sum += weight_[i][j];
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
			weight_[i][j] /= sum;
	normalized = 1;
}

void process_pixel( float *img , int img_w , int img_h , float value , int x , int y , int pixel_w )
{
	int x_,y_;
	float diff ;
	float sum = 0;
	if( x < 0 )
		return ;
	if( y < 0 ) 
		return ;
	for(y_ = 0; y_ < pixel_w && y+y_ < img_h ; y_++ )
		for(x_ = 0; x_ < pixel_w && x+x_ < img_w ; x_++ ) {
			sum += weight_[y_][x_]*img[(y+y_)*img_w + x+x_ ];
		}
	diff = lambda * ( value - sum );
	for(y_ = 0; y_ < pixel_w && y+y_ < img_h ; y_++ )
		for(x_ = 0; x_ < pixel_w && x+x_ < img_w ; x_++ ) {
			img[(y+y_)*img_w + x+x_ ] += diff*weight_[y_][x_];
			if( img[(y+y_)*img_w + x+x_ ] < 0 )
				img[(y+y_)*img_w + x+x_ ] = 0;
		}
}


int main( int argc , char **argv )
{
	FILE *f;
	int x,y;
	int i;
	int j;
	int loops = 0;
	int w,h;
	
	float *out_img = NULL;
	int zoom=1;
	int out_img_w;
	int out_img_h;
	char out_fname[1024];
	normalize();

	while( argv[1][0] == '-' ) {	
		if( strcmp(argv[1] , "-zoom" ) == 0 ) {
			argv++;
			argc--;

			zoom = atoi(argv[1] );
		}
	}
	while(1) {
		for(i=0;i<zoom;i++)
			for(j=0;j<zoom;j++) {
				char fname[256];
				sprintf(fname,"stack%d%d.pgm",i,j);
				unsigned short *p = read_pgm(&w,&h,fname );
				if(!p)
					continue;
				if( !out_img ) {
					out_img_w = w*zoom;
					out_img_h = h*zoom;
					out_img = malloc( out_img_w*out_img_h*sizeof(float) );
					memset( out_img , 0 , out_img_w*out_img_h*sizeof(float)  );
				}
				for(y=0;y<h;y+=1) {
					for(x=0;x<w;x+=1) {
						process_pixel( out_img , out_img_w , out_img_h , p[y*w+x] , x * zoom + j  , y * zoom + i  , 4   );
					}
				}
				free(p);
			}
		if( (loops % 10) == 0 ) {
			sprintf(out_fname,"out_img_%02d.pgm",loops/10);
			printf("write %s\n",out_fname);
			write_pgm(out_img,out_img_w,out_img_h,out_fname,0);
		}
		loops++;
	}
	
}


