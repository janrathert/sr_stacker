
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "gauss_distribution.h"
#include "sphere_transform.h"
#include "dct.h"

typedef double valtype;
#include "normalize.h"
int max_pixelvalue = 255;

float lambda = 1;
float gauss_width = 0;
float square_width = 0.5;

int focus = 0;

int blacklevel = 0;
long long maxdiff = 0;
int normalize_flag = 0;

// static int img_xo[100];
// static int img_yo[100];


typedef struct {
	char fname[1024];
	float x1,y1;
	float x2,y2;	
	float x3,y3;	
	long long diff1;
	long long diff2;
	long long diff3;
	float M[2][2];
	float A[2][2];
	float b[2];
	sphere_matrix mapping_3;
} Mapping; // M*x + b

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



/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
	SDL_Quit();
	exit(rc);
}

int lines_in_file( FILE *f )
{
	int count = 0;
	while(!feof(f)) {
		if( fgetc(f) == '\n' )
			count++;
	}
	fseek(f,0,SEEK_SET);
	printf("%d lines\n",count);
	return count;
}

valtype* read_pgm(  int *w , int *h,  const char *fname )
{
	int i;
	int j;
	FILE *f;
	int max;
	int w_,h_;
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
	printf("w %d h %d\n",w_,h_ );
	fgets(line,sizeof(line),f);
	sscanf(line,"%d",&max);
	max_pixelvalue = max;
	r = malloc( w_ * h_ * sizeof(valtype) );
	memset( r , 0 , w_ * h_ * sizeof(valtype) );
	for(i=0;i<h_;i++) {
		for(j=0;j<w_;j++) {
			unsigned char p1 = 0;
			int val;
			if( max > 255 )
				p1 = fgetc(f);
			unsigned char p2 = fgetc(f);
			val = ( p1*256 + p2 ) - blacklevel  ;
			if( val < 0 )
				val = 0;
			r[i*w_+j] = val;
		}
	}
	if(pipe)
		pclose(f);
	else
		fclose(f);
	*w = w_;
	*h = h_;
	printf("read_pgm = %d %d\n",w_,h_ );
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
	if( x<0 || y <0 || x>=w || y>=h )
		return 0;
	return img[y*w+x];
}


// #define filter_size 13

static float fp_[100*100][filter_size][filter_size] ;

static void normalize( int k )
{
	int i,j;
	float sum=0;
	FILE *f;

	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			sum += fp_[k][i][j];
		}
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			fp_[k][i][j] /= sum;
}


static void gen_fp_gauss( int k , float x0 , float y0 , float ro )
{
	int i;
	int j;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			fp_[k][i][j] = gauss_distribution2d( j-filter_size/2 , i-filter_size/2 , x0 ,y0 , ro );
	normalize(k);
}

static void gen_fp_rect( int k , float x0 , float y0 , float ro )
{
	int i;
	int j;
	for(i=0;i<filter_size;i++) {
		for(j=0;j<filter_size;j++) {
			fp_[k][i][j] = rect_distribution( j-filter_size/2 , i-filter_size/2 , x0 ,y0 , ro ); 
		}
	}
	
	normalize(k);
}



valtype* C;


valtype filter_sum( int k,  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if(x+j>=0 && y+i>=0 && x+j<w_ && y+i<h_)
				sum += fp_[k][i][j];

		}
	return sum;
}

void filter_sum_( int k, valtype* C,  int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if(x+j>=0 && y+i>=0 && x+j<w_ && y+i<h_)
				C[(y+i)*w_+x+j] += fp_[k][i][j];

		}
}

void map_point( Mapping *m , int x, int y , int *x_ , int *y_ , int *filter_idx )
{
	float xx;	
	float yy;
	int dx;
	int dy;
	if( focus ) {
		printf("focus mapping\n",focus);
		map_point3( &m->mapping_3 , &m->mapping_3 , x , y , &xx, &yy );
	} else {
		xx = m->M[0][0]*x + m->M[0][1]*y + m->b[0];
		yy = m->M[1][0]*x + m->M[1][1]*y + m->b[1];
	}
	*x_ = round( xx );
	*y_ = round( yy );
	dx = (xx - *x_)*100+50;
	dy = (yy - *y_)*100+50;
	if( dx >= 100 )
		dx = 99;
	if( dy >= 100 )
		dy = 99;
	if( dx < 0 || dx >= 100 || dy < 0 || dy >= 100 )
		printf("error dx=%d dy=%d xx=%f yy=%f\n",dx,dy,xx,yy); 
	
	*filter_idx =  dy*100+dx;
}



valtype forward_projection( int k, valtype *X , int w_,int h_,  int x , int y )
{
	int i,j;
	valtype sum = 0;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++)
			sum += fp_[k][i][j] * getpixel(X,w_,h_,x+j,y+i);
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

void back_projection( int k, valtype *X , valtype val  , int w, int h, int x , int y )
{
	int i,j;
	x -= filter_size/2;
	y -= filter_size/2;
	for(i=0;i<filter_size;i++)
		for(j=0;j<filter_size;j++) {
			if(x+j>=0 && y+i>=0 && x+j<w && y+i<h) {
				X[w*(y+i)+x+j] += C[(y+i)*w+x+j] * val * fp_[k][i][j]; 
				if( !( X[w*(y+i)+x+j] > -65000 
				   && X[w*(y+i)+x+j] < 65535*5 ) ) {
					printf("val = %f C = %f x=%d y=%d\n",val , C[(y+i)*w+x+j] ,x,y );
					exit(1);
				}
				
			}
		}
}

void register_mapping_3( Mapping *m , Mapping *m0 , int w , int h , float focus , float zoom )
{
	set_matrix3( m->x1 , m->y1 , m->x2 , m->y2 , w/2 , h/2 , focus , &m->mapping_3 , &m0->mapping_3 );
}

void register_mapping( Mapping *m , Mapping *m0 , float zoom )
{
			if( m->x2 ) {
				float dx = m->x2 - m->x1;
				float dy = m->y2 - m->y1;
				float dx_;
				float dy_;
				static float d;
				if( m == m0 )
					d = sqrt(dx*dx+dy*dy);
				dx /= d;
				dy /= d;
				dx_ = dy;
				dy_ = -dx;
				printf("dx = %f dy = %f x1 = %f y1 = %f\n",dx,dy , m->x1,m->y1 );
				if( m == m0 ) {
					m->A[0][0] = dx;
					m->A[0][1] = dy;
					m->A[1][0] = dx_;
					m->A[1][1] = dy_;
				}
				{ float A[2][2] = {
					{dx,dy },
					{dx_,dy_} };
				  float B[2][2] = {
					{ m0->A[0][0]*zoom,m0->A[1][0]*zoom },
					{ m0->A[0][1]*zoom,m0->A[1][1]*zoom } };
				  matrix_mult_2(m->M,A,B );
				}
				
			} else {
				m->M[0][0] = zoom;
				m->M[1][1] = zoom;

			}
			matrix_mult_vector_2(m->b, m->M , -m->x1 , -m->y1 );

			m->b[0] += m0->x1*zoom;
			m->b[1] += m0->y1*zoom;
}

void check_img( valtype* img , int w, int h , const char *s )
{
	int size = w*h;
	int i;
	for(i=0;i<size;i++)
		if( !(img[i]>-65535 && img[i]<5*65535 ) ) {
			printf("check img failed %s %f\n",s,img[i]);
			exit(1);
		}
}


int round_rand( int loops , int lr_count )
{
	static unsigned int mask[32];
	static int i;
	static int loops_ = -1;
	if( loops == loops_ )
		return i;
	loops_ = loops;
	// printf("round_rand loops=%d count=%d\n",loops,lr_count);
	if( loops % lr_count == 0 )
		memset(mask,0,sizeof(mask));
	while(1) {
		i = rand()%lr_count;
		if( mask[i/32] & (1<<i) )
			continue;
		mask[i/32] |= 1<<i;
		break;
	}
	// printf("ret i=%d\n",i);
	return i;
}
#define FOR_LR(i) for(i=1;i<lr_count;i++) 
// #define check_img(img,w,h,s)
int main( int argc , char **argv )
{
	FILE *f;
	int x,y;
	int i;
	int j;
	int k;
	int loops = 0;
	int w,h;
	int w_,h_;
	float xo0 = -1;
	float yo0 = 0;
	float dx0 = 1;
	float dy0 = 0;
	float dx0_ = 0;
	float dy0_ = 1;
	
	int zoom=1;
	int out_img_w;
	int out_img_h;
	int lr_count = 0;
	char out_fname[1024];
	char path[1024];
	valtype **b;
	
	Mapping *map;
	valtype threshold = 0;
	valtype threshold2 = 0;
	char channel = 0;
	int max_loops = 20;
	int reverse = 0;
	valtype *X;
	valtype img_avg;
	valtype img_mean_deviation;
	int max_images;
	strcpy(out_fname,"out.pgm");	

	while( argc >= 2 && argv[1][0] == '-' ) {	
		if( strcmp(argv[1] , "-zoom" ) == 0 ) {
			argv++;
			argc--;

			zoom = atoi(argv[1] );
		}
		if( strcmp(argv[1] , "-focus" ) == 0 ) {
			argv++;
			argc--;

			focus = atoi(argv[1] );

		}
		if( strcmp(argv[1] , "-normalize" ) == 0 ) {
			normalize_flag = 1;
		}
		if( strcmp(argv[1] , "-loops" ) == 0 ) {
			argv++;
			argc--;

			max_loops = atoi(argv[1] );
		}
		if( strcmp(argv[1] , "-maxdiff" ) == 0 ) {
			argv++;
			argc--;
			maxdiff = atoi( argv[1] );
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
		X =  read_pgm(&w_,&h_,argv[2] );
		w = w_/zoom;
		h = h_/zoom;
	}
		
	{ int dx,dy;
	  for(dy=0;dy<100;dy++) {
		printf("dy=%d\n",dy);
		for(dx=0;dx<100;dx++) {
			if(square_width)
				gen_fp_rect( dy*100+dx , dx/100.0-0.5  , dy/100.0-0.5  , square_width/2.0  );	
			else
				gen_fp_gauss( dy*100+dx , dx/100.0-0.5  , dy/100.0-0.5  ,  gauss_width  );	
		}
	  }
	}
	printf("print filter\n");

	k = 50*100+75;
	for(i=0;i<filter_size;i++) {
		for(j=0;j<filter_size;j++)
			printf("%9.3f ",fp_[k][i][j] );
		printf("\n");

	}

	{ char *p;
	  strcpy(path,argv[1]);
	  p = rindex(path,'/');
	  if(p)
		p[1]=0;
	  else
	       path[0]=0;
	}
	printf("path=%s\n",path);
		
	f = fopen(argv[1],"r");
	if(!f) {
		printf("can't open %s\n",argv[1] );
	}
	printf("get lines in file\n");
	max_images = lines_in_file(f)+1;
	b = malloc( sizeof(valtype*) * max_images );
	map = malloc( sizeof(Mapping) * max_images );
	while(!feof(f) ) {
		long long diff;
		long long diff2;
		int ret;
		int ref  = 0;
		char fname[1024] ;
		char line[1024] = { 0 };
		char *line_ = line;
		Mapping* m = map+lr_count;
		Mapping* m0 = map;
		fgets( line , 1024 , f );
		memset(m,0,sizeof(*m) );
		if( strncmp(line,"#ref:",5) == 0 ) {
			ref = 1;
			line_ = line+6;
		} else if( line[0] == '#' )
			continue;
		ret =  sscanf(line_,"%s %f %f %lld %f %f %lld %f %f %lld",m->fname,&m->x1,&m->y1,&m->diff1,&m->x2,&m->y2,&m->diff2,&m->x3,&m->y3,&m->diff3);
		if( ret < 4 ) 
			break;
		printf("img %d fname %s \n",lr_count,m->fname );
		if( maxdiff &&  m->diff1 > maxdiff ) {
			printf("%s %lld ignored\n",m->fname,m->diff1 );
			continue;
		}
		strncpy(fname,path,sizeof(fname));
		strncat(fname,m->fname,sizeof(fname));
		if( channel ) {
			char *p = rindex(fname,'_');
			if( p[-2] == 'r' || p[-2] == 'g' || p[-2] == 'b' )
				p[-2] = channel;
			else {
				printf("error replacing channel\n");
			}
		}
		if(!ref) {
			if(!reverse) {
				float avg,sigma;
				b[lr_count] = read_pgm(&w,&h,fname );
				check_img(b[lr_count],w,h,"after read");
				normalize_img( b[lr_count] , w ,h , &avg,&sigma,0,0 );
				printf("avg = %f sigma = %f %f\n",avg,sigma,img_mean_deviation );
				if( lr_count == 1 ) {
					printf("first\n");
					normalize_img( b[lr_count] , w ,h , &img_avg,&img_mean_deviation,65536,0 );
					printf("first %f\n",img_mean_deviation);
					if( img_mean_deviation < 1 ) {
						printf("failed\n");
						exit(1);
					}
				} else {
					if( normalize_flag )
						normalize_img( b[lr_count] , w ,h , NULL,NULL, 65536,img_mean_deviation );
				}
				printf("avg = %f sigma = %f %f\n",avg,sigma,img_mean_deviation );
				check_img(b[lr_count],w,h,"after normalize");
			} else
				b[lr_count] = malloc(w*h*sizeof(valtype) );
		} else {
			b[lr_count] = 0;
		}
		printf("w = %d h = %d\n",w,h);
		lr_count++;
	}
	fclose(f);
	if( focus == 1 ) {
		float foc;
		float min = 1000;
	  	for( foc = w*100  ; foc > w || foc > h ;  foc-- ) {
			Mapping* m = map+lr_count-1;
			Mapping* m0 = map;
			float xx,yy;
			float diff;
			register_mapping_3(m0,m0,w,h,foc,zoom);
			register_mapping_3(m,m0,w,h,foc,zoom);
			map_point3( &m->mapping_3 , &m->mapping_3 , m->x3 , m->y3 , &xx, &yy );
			diff = (xx-m0->x3)*(xx-m0->x3) + (yy-m0->y3)*(yy-m0->y3);
			if( diff < min ) {
				min = diff;
				focus = foc;
			}

	  	}

	}
	printf("focus = %d\n",focus );
	for( i=0;i<lr_count;i++) {
		Mapping* m  = map+i;
		Mapping* m0 = map;
		if( focus ) 
			register_mapping_3(m,m0,w,h,focus,zoom);
		else
			register_mapping(m,m0,zoom);
		if( reverse ) {
			char fname[1024] ;
			for(y=0;y<h;y++) {
				for(x=0;x<w;x++) {
					int x_,y_;
					int filter_idx;
					map_point(m,x,y,&x_,&y_,&filter_idx);
					b[i][y*w+x] =  forward_projection( filter_idx , X , w_,h_,  x_ , y_ );
				}
				// printf("\n");
			}
			write_pgm(b[i],w,h,m->fname,0);
		}
	}
	if( reverse )
		exit(0);
	threshold = 0; //  max_pixelvalue * 0.001;
	threshold2 =  max_pixelvalue * 0.001;
	w_ = zoom*w;
	h_ = zoom*h;
	X = malloc(w_*h_*sizeof(valtype) );
	C = malloc(w_*h_*sizeof(valtype) );
	memset( X , 0 , w_*h_*sizeof(valtype) );
	memset( C , 0 , w_*h_*sizeof(valtype) );
	printf("lr_count = %d\n",lr_count );
	FOR_LR(i) {
		printf("i=%d\n",i);
		check_img(b[i],w,h,"before filter_sum");
		for(y=0;y<h;y++)
			for(x=0;x<w;x++) {
				int x_,y_;
				int filter_idx;
				map_point(&map[i],x,y,&x_,&y_,&filter_idx);
				filter_sum_( filter_idx ,C,w_,h_,x_,y_);
			}
	}
	for(i=0;i<w_*h_;i++)  {
		if( !C[i] ) {
			printf("null i=%d\n",i);		
			C[i] = 0;
		} else { 
			C[i] = 1/C[i];
		}
	}

	{
		FOR_LR(i) {
			printf("i=%d\n",i);
			check_img(b[i],w,h,"before projection");
			for(y=0;y<h;y++)
				for(x=0;x<w;x++) {
					int x_,y_;
					int filter_idx;
					map_point(&map[i],x,y,&x_,&y_,&filter_idx);
					back_projection( filter_idx , X , b[i][w*y+x] , w_,h_, x_,y_ );
				}
		}
	}
	if ( channel )
		sprintf(out_fname,"out_%c.pgm",channel);	
	printf("out image %s\n",out_fname );
	write_pgm(X,w_,h_,out_fname,0);
	
}


