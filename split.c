
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
	if(!v)
		return 1;

	v += w*y+x;
	return *v;
}

int filter_size = 5;

float fp_[5][5] = {
	{  1 , 1, 1 },
	{  1 , 1, 1 },
	{  1 , 1, 1  }
};
typedef unsigned short valtype;


static void normalize()
{
	int i,j;
	float sum=0;
	FILE *f;
	f = fopen("gauss","r");

	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if(f)
				fscanf(f,"%f", &fp_[i][j] );
			sum += fp_[i][j];
		}
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			fp_[i][j] /= sum;
	if(f)
		fclose(f);
}

valtype filter_sum(  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if(x+j>=0 && y+i>=0 && x+j<w_ && y+i<h_)
				sum += fp_[i][j];

		}
	return sum;
}

void filter_sum_( valtype* C,  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if(x+j>=0 && y+i>=0 && x+j<w_ && y+i<h_)
				C[(y+i)*w_+x+j] += fp_[i][j];

		}
}


float forward_projection( valtype *X , int w_,int h_,  int x , int y )
{
	int i,j;
	float sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			sum += fp_[i][j] * getpixel_ushort(X,w_,h_,x+j,y+i);

	return sum;
}


int main( int argc, char **argv )
{
	
	int w,h;
	int x,y;
	unsigned short *p = read_pgm ( &w,&h , argv[1] , NULL , NULL );
	int *s = malloc( w*h* sizeof(int) );

	int i,j;
	int k,l;
	normalize();

	for( i = 0;i<2; i++ )
		for( j=0;j<2; j++ ) {
			char fname[256];
			sprintf(fname,"stack%d%d.pgm",i,j);
			printf("i=%d j=%d\n",i,j);

			for(y=0;y<h/2;y+=1) {
				for(x=0;x<w/2;x+=1) {
					s[y*(w/2)+x] = forward_projection( p ,w,h, 2*x+j,2*y+i ) / forward_projection(NULL,w,h,2*x+j,2*y+i);  
				}
			}
			write_pgm_stack(s,w/2,h/2,fname, 1 );
		}
			

}



