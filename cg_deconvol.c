
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "gauss_distribution.h"
#include "sphere_transform.h"
#include "dct.h"

typedef double valtype;
int max_pixelvalue = 255;

float lambda = 1;
float gauss_width = 1;
float square_width = 0;

int focus = 0;
int psf = 0;
int psf_pgm = 0;

int blacklevel = 0;

int maxdiff = 0;

#define MAX_IMAGES 1
#define MAX_FILTERSIZE 99



void matrix_mult_2( float R[2][2] , float A[2][2] , float B[2][2] ) // R = A*B
{
	int i;
	int j;
	for(i=0;i<2;i++)
		for(j=0;j<2;j++)
			R[i][j] = A[i][0]*B[0][j] + A[i][1] * B[1][j] ;
}

void matrix_mult_vector_2( float r[2], float A[2][2] , float b0 , float b1 ) // r = A*b
{
	int i;
	int j;
	for(i=0;i<2;i++)
		r[i] = A[i][0]*b0 + A[i][1] * b1 ;
}


valtype* read_pgm(  int *w , int *h,  const char *fname )
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
	valtype *r;
	if( strcmp(fname+strlen(fname)-4,".pgm") != 0 ) {
		char cmd[1024];
		sprintf(cmd,"convert -channel green -geometry 1560x2080  %s pgm:-",fname);
		// sprintf(cmd,"convert %s pgm:-",fname);
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
	printf("w %d h %d\n",w_,h_ );
	fgets(line,sizeof(line),f);
	sscanf(line,"%d",&max);
	max_pixelvalue = max;
	r = malloc( w_ * h_ * sizeof(valtype) );
	memset( r , 0 , w_ * h_ * sizeof(valtype) );
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
			int val;
			if( max > 255 )
				p1 = fgetc(f);
			unsigned char p2 = fgetc(f);
			val = ( p1*256 + p2 ) - blacklevel  ;
			if( val < 0 )
				val = 0;
			if( max_pixelvalue == 255 )
				val *= 256;

			// val /= 2;
			
			r[i*w_+j] = val;
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
	printf("read_pgm = %d %d\n",w_,h_ );
	max_pixelvalue = 65535;
	return r;
}

void write_pgm( valtype *out_img , int w , int h , char *fname , int inv )
{
	FILE *f;
	int x,y;
	int mult = 1;
	if( max_pixelvalue == 255 )		
		mult = 256;
	f = fopen(fname,"w");
	fprintf(f,"P5\n%d %d\n%d\n",w,h,65535);
	for(y=0;y<h;y+=1)
		for(x=0;x<w;x+=1) {
			int value =  out_img[y*w+x]*mult+0.5 ;
			if( inv )
				value = max_pixelvalue - value;

			if( value < 0 )
				value = 0;
			
			if( value > 65535 ) {
				printf("pixel %d %d = %d !\n",x,y,value );
				value = 65535;
			}
			fputc( value/256 , f);
			fputc( value&255 , f);
		}
	fclose(f);
}



valtype getpixel( valtype *img , int w, int h, int x , int y )
{
	if( x<0 )
		x=0;
	if( y<0)	
		y=0;
	if(  x>=w  )
		x = w-1;
	if(  y>=h  )
		y = h-1;
	return img[y*w+x];
}



float fp_[MAX_FILTERSIZE][MAX_FILTERSIZE] ;
int filter_size = 0;

static void normalize(  )
{
	int i,j;
	float sum=0;
	FILE *f;

	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			sum += fp_[i][j];
		}
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			fp_[i][j] /= sum;
}


static void gen_fp_gauss(  float x0 , float y0 , float ro )
{
	int i;
	int j;
	filter_size = 9;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			fp_[i][j] = gauss_distribution2d( j-filter_size/2 , i-filter_size/2 , x0 ,y0 , ro );
	normalize();
}

static void gen_fp_rect( float x0 , float y0 , float ro )
{
	int i;
	int j;
	filter_size = 9;
	for(i=0;i<filter_size;i++) {
		for(j=0;j<filter_size;j++) {
			fp_[i][j] = rect_distribution( j-filter_size/2 , i-filter_size/2 , x0 ,y0 , ro ); 
		}
	}
	
	normalize();
}

static void gen_fp_pgm( const char *fname  )
{
	int i;
	int j;
	int w,h;
	valtype *p;
	valtype *img = read_pgm( &w , &h , fname );
	printf("gen_fp_pgm img = %p\n",img );
	if(!img) {
		printf("psf pgm not found\n");
		exit(1);
	}
	p = img;
	filter_size = w;
	for(i=0;i<filter_size;i++) {
		for(j=0;j<filter_size;j++) {
			fp_[i][j] = *p++;
		}
	}
	/*
	for(i=0;i<filter_size;i++) {
		for(j=0;j<filter_size;j++) {
			fp_[i][j] = (fp_[i][j] +  *--p)/2;
		}
	}
	*/

	free(img);
	
	normalize();
}



static void gen_fp_psf(  float x0 , float y0 , const char *fname  )
{
	int i;
	int j;
	FILE *f;
	float v[100];
	float black;
	f = fopen(fname,"r");
	for(i=0;i<100;i++) {
		float x;
		if( fscanf(f,"%f %f",&x,&v[i] ) != 2 ) 
			break;
		black = v[i];
	}
	filter_size=i/2;
	if( !(filter_size & 1) )
		filter_size--;
	printf("filter_size = %d\n",filter_size );
	for(i=0;i<filter_size;i++) {
		for(j=0;j<filter_size;j++) {
			float x = j-filter_size/2; 
			float y = i-filter_size/2; 
			int r2 = floor(  sqrt( x*x + y*y )*2 + 0.5 );
			if( r2>filter_size )
				fp_[i][j] = 0 ; 
			else
				fp_[i][j] = v[r2] - black ; 
		}
	}
	fclose(f);
	
	normalize();
}







valtype filter_sum(  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			sum += fp_[i][j];

		}
	return sum;
}

#if 0
void filter_sum_(  valtype* C,  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if( x+j>=0 && x+j<w_ && y+i>=0 && y+i<h_)
				C[(y+i)*w_+x+j] += fp_[i][j];

		}
}


void transposed_filter_sum_(  valtype* C,  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if( x+j>=0 && x+j<w_ && y+i>=0 && y+i<h_)
				C[(y+i)*w_+x+j] += fp_[filter_size-1-i][filter_size-1-j];

		}
}

#endif

valtype forward_projection(  valtype *X , int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			sum += fp_[i][j] * getpixel(X,w_,h_,x+j,y+i);
	if( !(sum >= 0 || sum < 0 ) ) {
		printf("nan\n");
		exit(1);
	}

	return sum;
}

valtype transposed_forward_projection(  valtype *X , int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			sum += fp_[filter_size-1-i][filter_size-1-j] * getpixel(X,w_,h_,x+j,y+i);
	if( !(sum >= 0 || sum < 0 ) ) {
		printf("nan\n");
		exit(1);
	}

	return sum;
}


int check_nan( float x , const  char *s )
{
	if( !(x>=0 || x<0 ) ) {
		printf("is nan %s\n", s );
		return 1;
	}
	return 0;
}


int main( int argc , char **argv )
{
	FILE *f;
	int x,y;
	int i;
	int j;
	int loops = 0;
	int w,h;
	float xo0 = -1;
	float yo0 = 0;
	float dx0 = 1;
	float dy0 = 0;
	float dx0_ = 0;
	float dy0_ = 1;
	
	int out_img_w;
	int out_img_h;
	char out_fname[1024];
	valtype *b;
	valtype *r;
	valtype *z;
	valtype *p;
	valtype *q;
	valtype t = 1;
	valtype t_ = 1;
	valtype beta;
	
	valtype threshold = 0;
	valtype threshold2 = 0;
	char channel = 0;
	int max_loops = 20;
	int reverse = 0;
	valtype *X;
	strcpy(out_fname,"out.pgm");	

	while( argv[1][0] == '-' ) {	
		if( strcmp(argv[1] , "-focus" ) == 0 ) {
			argv++;
			argc--;

			focus = atoi(argv[1] );

		}
		if( strcmp(argv[1] , "-loops" ) == 0 ) {
			argv++;
			argc--;

			max_loops = atoi(argv[1] );
		}
		if( strcmp(argv[1] , "-blacklevel" ) == 0 ) {
			argv++;
			argc--;

			blacklevel = atoi(argv[1] );
		}
		if( strcmp(argv[1] , "-gauss_width" ) == 0 ) {
			argv++;
			argc--;

			gauss_width = atof(argv[1] );
		}
		if( strcmp(argv[1] , "-square" ) == 0 ) {
			argv++;
			argc--;

			square_width = atof(argv[1] );
		}
		if( strcmp(argv[1] , "-psf" ) == 0 ) {
			psf = 1;
		}
		if( strcmp(argv[1] , "-psf_pgm" ) == 0 ) {
			psf_pgm = 1;
		}
		if( strcmp(argv[1] , "-lambda" ) == 0 ) {
			argv++;
			argc--;

			lambda = atof(argv[1] );
		}
		if( strcmp(argv[1] , "-channel" ) == 0 ) {
			argv++;
			argc--;

			channel = argv[1][0] ;
		}
		if( strcmp(argv[1] , "-o" ) == 0 ) {
			argv++;
			argc--;
			strcpy(out_fname,argv[1]);
		}
		if( strcmp(argv[1] , "-reverse" ) == 0 ) {
			reverse = 1;
		}
		argv++;
		argc--;
	}
	if( reverse ) {
		X =  read_pgm(&w,&h,argv[2] );
	}
	if(psf) {
		char fname[1024];
		strcpy(fname,argv[1]);
		char *p = index(fname,'.');
		if( p ) {
			strcpy(p,"_psf.txt");
			gen_fp_psf( 0 , 0 , fname   );		
		}
	} else if(psf_pgm) {
		char fname[1024];
		strcpy(fname,argv[1]);
		char *p = index(fname,'.');
		if( p ) {
			strcpy(p,"_psf.pgm");
			gen_fp_pgm(  fname   );		
		}
	}
	else if(square_width)
		gen_fp_rect( 0,0  , square_width/2.0  );	
	else if(square_width)
		gen_fp_rect( 0,0  , square_width/2.0  );	
	else
		gen_fp_gauss( 0,0  ,  gauss_width  );	
	if(!reverse)
		b = read_pgm(&w,&h,argv[1] );
	printf("w = %d h = %d\n",w,h);
	r = malloc(w*h*sizeof(valtype) );
	z = malloc(w*h*sizeof(valtype) );
	if( reverse ) {
		for(y=0;y<h;y++) {
			for(x=0;x<w;x++) {
				int x_,y_;
				x_ = x;
				y_ = y;
				r[y*w+x] =  forward_projection(  X , w,h,  x_ , y_ );
				// printf("%f ",r[y*w+x] );
			}
			// printf("\n");
		}
		write_pgm(r,w,h,argv[1],0);
	}

	if( reverse )
		exit(0);
	threshold = 0; //  max_pixelvalue * 0.001;
	// threshold2 = max_pixelvalue * 0.001;
	X = malloc(w*h*sizeof(valtype) );
	p = malloc(w*h*sizeof(valtype) );
	q = malloc(w*h*sizeof(valtype) );
	valtype *X_ = malloc(w*h*sizeof(valtype) );
	valtype *X__ = malloc(w*h*sizeof(valtype) );
	memset( X , 0 , w*h*sizeof(valtype) );

	/*
	printf("b = A^T * b\n");

	for(y=0;y<h;y++)
		for(x=0;x<w;x++) {
			r[y*w+x] = transposed_forward_projection(  b , w,h,  x , y );
		}
	memcpy( b , r , w*h*sizeof(valtype) );
	*/

	printf("start loop\n");

	while(loops<=max_loops) {
		 if( loops == 0  )
		 {
			memcpy( r , b , w*h*sizeof(valtype) );
			// b - A*X
			{
				/*
				for(y=0;y<h;y++)
					for(x=0;x<w;x++) {
						X__[y*w+x] = forward_projection(  X , w,h,  x , y );
					}
				*/
				for(y=0;y<h;y++)
					for(x=0;x<w;x++) {
						r[y*w+x] -= forward_projection(  X, w,h,  x , y );
					}
			}
			if( loops == 0 ) {
				memcpy( p , r , w*h*sizeof(valtype) );
			}
		}
		// solve M*z = r , here M=I
		{
			memcpy( z , r , w*h*sizeof(valtype) );
		}
		t_ = t;
		t = 0;
		{
			for(y=0;y<h;y++)
				for(x=0;x<w;x++) {
					t += r[y*w+x]*z[y*w+x];
				}
		}
		if( loops == 0 )  {
			{
				for(y=0;y<h;y++)
					for(x=0;x<w;x++) {
						p[y*w+x]=z[y*w+x];
					}
			}
		} else {
			if( t_ == 0 ) {
				printf("t_ = 0 !!\n");
				break;
			}
			beta = t/t_;
			printf("beta = %f t = %f t_ = %f\n",beta,t,t_ );
			{
				for(y=0;y<h;y++)
					for(x=0;x<w;x++) {
						p[y*w+x]= z[y*w+x] + beta*p[y*w+x];
					}
			}

		}
		{
				for(y=0;y<h;y++)
					for(x=0;x<w;x++) {
						q[y*w+x] = forward_projection(  p , w,h,  x , y );
					}
			/*
			for(y=0;y<h;y++)
				for(x=0;x<w;x++) {
					q[y*w+x] = transposed_forward_projection(  X__ , w,h,  x , y );
				}
			*/
		}
		{ valtype tmp = 0;
		  int j;
		  valtype a;
		  valtype max_r = 0;
		  for(j=0;j<w*h;j++)
			tmp += p[j]*q[j];
		  if( tmp == 0 ) {
			printf("p*q = 0 !!\n");
			break;
		  }
		  a = t/tmp;
		  printf("a = %f\n",a);
		  for(j=0;j<w*h;j++) {
			X[j] += lambda * a * p[j];
			r[j] -= lambda*a*q[j];
			if( fabs(r[j]) > max_r )
				max_r = fabs(r[j] );
			/*
			if(X[j]<0) {
				X[j] = 0;
			}
			if(X[j]>max_pixelvalue)
				X[j] = max_pixelvalue;
			*/
		  }
			
			printf("iteration %d max_r %f\n",loops , max_r );
		}
		loops++;
	}
	int negative_count = 0;
	for(j=0;j<w*h;j++) {
		if( X[j] < 0 )
			negative_count++;
	}
	if ( channel )
		sprintf(out_fname,"out_%c.pgm",channel);	
	printf("out image %s negative %d\n",out_fname , negative_count );
	write_pgm(X,w,h,out_fname,0);
	
}


