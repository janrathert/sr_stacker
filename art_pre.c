#include <stdio.h>
#include <math.h>
#include "circle.h"
#include "image_match.h"

int max_pixelvalue = 255;

float lambda = 0.1;
int min_x = 0;
int min_y = 0;
int max_x = 0;
int max_y = 0;

int black_level = 0;


static int *stack_images[16][16];
static int stack_count[16][16] ;



unsigned short* read_pgm(  int *w , int *h,  const char *fname , int *x_offset , int *y_offset )
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
		if(x_offset && y_offset && strncmp(line,"#offset ",8) == 0 )
			sscanf(line+8,"%d %d",x_offset,y_offset);
	} while(line[0] == '#' );
	sscanf(line,"%d %d",&w_,&h_);
	w_ -= 2*cropw;
	h_ -= 2*croph;
	if(!max_x) {
		max_x = w_;
		max_y = h_;
	}
	printf("line = %s\n",line);
	fgets(line,sizeof(line),f);
	sscanf(line,"%d",&max);
	printf("line = %s\n",line);
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
			int v;
			if( max > 255 )
				p1 = fgetc(f);
			unsigned char p2 = fgetc(f);
			v = ( p1*256 + p2 ) - black_level  ;
			if( v < 0 )
				v = 0;
			r[k++] = v  ;
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

void write_pgm_stack( int *out_img , int w , int h , char *fname , int count  )
{
	FILE *f;
	int x,y;
	if(!count)
		return;
	f = fopen(fname,"w");
	fprintf(f,"P5\n%d %d\n%d\n",w,h,max_pixelvalue);
	for(y=0;y<h;y+=1)
		for(x=0;x<w;x+=1) {
			int value =  out_img[y*w+x] / count ;
			if( value < 0 )
				value = 0;
			if( value > max_pixelvalue )
				value = max_pixelvalue;
			if( max_pixelvalue > 255 )
				fputc( value/256 , f);
			fputc( value&255 , f);
		}
	fclose(f);
}

void write_pgm_ushort( unsigned short *out_img , int w , int h , char *fname , int x_ , int y_ , int w_ , int h_   )
{
	FILE *f;
	int x,y;
	f = fopen(fname,"w");
	fprintf(f,"P5\n#offset %d %d\n%d %d\n%d\n",x_,y_,w_,h_,max_pixelvalue);
	for(y=y_;y<y_+h_;y+=1)
		for(x=x_;x<x_+w_;x+=1) {
			int value =  out_img[y*w+x]  ;
			if( value < 0 )
				value = 0;
			if( value > max_pixelvalue )
				value = max_pixelvalue;
			if( max_pixelvalue > 255 )
				fputc( value/256 , f);
			fputc( value&255 , f);
		}
	fclose(f);
}


void write_pgm_s( unsigned short *out_img , int w , int h , char *fname , int idx )
{
	FILE *f;
	int x,y;
	char fname2[1024];
	sprintf(fname2,fname,idx);
	f = fopen(fname2,"w");
	fprintf(f,"P6\n%d %d\n%d\n",max_x-min_x,max_y-min_y,255);
	for(y=min_y;y<max_y;y+=1)
		for(x=min_x;x<max_x;x+=1) {
			int value =  out_img[y*w+x] ;
			int r,g,b;
			value *= 4;
			r =  8*(value/(32*32)) ;
			g =  8*((value/32)&31) ;
			b =  8*(value&31) ;
			if( value && r==0 && g==0 && b==0 ) {
				printf("error %d %d %d %d\n",value,r,g,b);
				exit(1);
			}
			fputc( r  , f);
			fputc( g  , f);
			fputc( b  , f);
		}
	fclose(f);
}


unsigned short getpixel_ushort( unsigned short *v , int w , int h , int x , int y )
{
	if( y < 0 || x < 0 || x >= w || y >= h ) 
		return 0;
	v += w*y+x;
	return *v;
}

// d  * r  = x

void process_pixel( float *img , int img_w , int img_h , float value , int x , int y , int pixel_w )
{
	int x_,y_;
	float sum = 0;
	int weight = 0;
	float diff ;
	if( x < 0 )
		return ;
	if( y < 0 ) 
		return ;
	for(y_ = 0; y_ < pixel_w && y+y_ < img_h ; y_++ )
		for(x_ = 0; x_ < pixel_w && x+x_ < img_w ; x_++ ) {
			sum += img[(y+y_)*img_w + x+x_ ];
			weight++;
		}
	if( weight == 0 )
		return;
	sum /= weight;
	diff = lambda * ( value - sum );
	for(y_ = 0; y_ < pixel_w && y+y_ < img_h ; y_++ )
		for(x_ = 0; x_ < pixel_w && x+x_ < img_w ; x_++ ) {
			img[(y+y_)*img_w + x+x_ ] += diff;
		}
}



int main( int argc,char **argv )
{
	FILE *f;
	int x,y;
	int i;
	int j;
	int loops = 0;
	int div=50;
	float max= 0;
	int x_max = 0;
	int y_max = 0;
	int x_max2 = 0;
	int y_max2 = 0;
	float cx0;
	float cy0;
	float gx = 0;
	float gy= 0;
	float gx2 = 0;
	float gy2= 0;
	float threshold2 = 0;
	float min_g = 10;
	float max_ = 0;
	int j_max = -1;
	int k_max;
	int j2_max;
	int k2_max;
	float z1 = 0 ,z2 = 0;
	float z1_ = 0 ,z2_ = 0;
	float dx[100] , dy[100];
	float dx_ , dy_;
	int w,h;
	unsigned short *first_img = NULL;
	int rel_needle_x = 60;
	int rel_needle_y = 60 ;
	int needle_x = 0;
	int needle_y = 0;
	
	float *out_img;
	int zoom=1;
	char out_fname[1024];
	int bad[100];
	float xo0 = 0,yo0 = 0;
	dx[0] = 0;
	dy[0] = 0;


	while( argv[1][0] == '-' ) {	
		if( strcmp(argv[1] , "-zoom" ) == 0 ) {
			argv++;
			argc--;

			zoom = atoi(argv[1] );
			argv++;
			argc--;
		} else {
			printf("invalid arg %s\n",argv[1]);
			exit(1);
		}
	}
			
		
	f = fopen(argv[1],"r");
	while(!feof(f) ) {
		float xo_,yo_;
		long long diff;
		char fname[1024] ;
		if( fscanf(f,"%s %f %f %lld",fname,&xo_,&yo_,&diff) != 4 )
			break;
		if(!xo0) {
			xo0 = xo_;
			yo0 = yo_;
		}
		unsigned short *p = read_pgm(&w,&h,fname , NULL , NULL  );
		int dxi,dyi;
		int dxi_,dyi_;
		if( !first_img ) {
			for( y=0; y<zoom;y++)
				for( x=0; x<zoom;x++) {
					stack_images[y][x] = malloc( w*h*sizeof(int) );
					memset( stack_images[y][x] , 0 , w*h*sizeof(int) );
				}
			first_img = p;
		}
		dxi = 1024 + (xo_-xo0)*zoom + 0.5;
		dyi = 1024 + (yo_-yo0)*zoom + 0.5;
		dxi_ = dxi%zoom;
		dyi_ = dyi%zoom;
		dxi = xo_ - xo0;
		dyi = yo_ - yo0;
		
		printf("i = %d dxi_ = %d dyi_ = %d dxi=%d dyi=%d %s w=%d h=%d\n",i,dxi_,dyi_ , dxi,dyi , fname ,w,h );
		if( p ) {
			int *s = stack_images[dyi_][dxi_];
			for(y=0;y<h;y+=1) {
				for(x=0;x<w;x+=1) {
					s[y*w+x] += getpixel_ushort( p , w,h, x+dxi,y+dyi );
				}
			}
			
			stack_count[dyi_][dxi_]++;
			free(p);
		}
	}
	printf("stack counts\n");
	for( y=0; y<zoom;y++) {
		for( x=0; x<zoom;x++) {
			printf("%02d ",stack_count[y][x] );
			char fname[256];
			sprintf(fname,"stack%d%d.pgm",y,x);
			unlink(fname);
			write_pgm_stack(stack_images[y][x],w,h,fname,stack_count[y][x] );
		}
		printf("\n");
	}

	exit(0);
	
}

