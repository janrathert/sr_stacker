#include <stdio.h>
#include <math.h>

static float get_pixel( unsigned short *img, int w , int h, float x , float y )
{
	int x_,y_;
	float v1,v2;
	float wx,wy;
	if( x < 0 || y < 0 || x >= w-1 || y >= h-1 )
		return 0;
	x_ = floor(x);
	y_ = floor(y);
	wx = x - x_;
	wy = y - y_;
	v1 = (1-wx) * img[y_*w+x_] + wx*img[y_*w+x_+1];
	v2 = (1-wx) * img[(y_+1)*w+x_] + wx*img[(y_+1)*w+x_+1];
	return (1-wy) * v1 + wy*v2;
}

static float get_pixel_2x2( unsigned short *img, int w , int h, float x , float y )
{
	int avg = 0;
	avg = get_pixel(img,w,h,x,y);
	avg += get_pixel(img,w,h,x+1,y);
	avg += get_pixel(img,w,h,x,y+1);
	avg += get_pixel(img,w,h,x+1,y+1);
	return avg/4;
}

unsigned short *get_sub_image( unsigned short *v , int w , int h , float cx , float cy , int r , float pixelstep  )
{
	int i;	
	int j;
	int w_ = 2*r;
	int h_ = 2*r;

	unsigned short *sub = malloc( w_*h_*sizeof(short));
	
	int k=0;
	if( pixelstep < 2.5  ) {
		for( i = -r; i< r ; i++  ) {
			for( j = -r; j< r ; j++  ) {
				int val =  round( get_pixel(v,w,h,cx+j*pixelstep,cy+i*pixelstep ) );
				sub[k++] = val;
			}
		}
	} else {
		for( i = -r; i< r ; i++  ) {
			for( j = -r; j< r ; j++  ) {
				int val =  round( get_pixel_2x2(v,w,h,cx+j*pixelstep,cy+i*pixelstep ) );
				sub[k++] = val;
			}
		}

	}
	return sub;
}

unsigned short *downsample( unsigned short *v , int w , int h , int xo , int yo , int dw , int dh , int scale   )
{
	int x,y;
	int i,j;
	unsigned short *r = malloc( dw*dh*sizeof(short) );
	unsigned short *p = r;
	v += yo*w + xo;
	for(y=0;y<dh;y++) {
		for(x=0;x<dw;x++) {
			int sum = 0;
			for(i=0;i<scale;i++)
				for(j=0;j<scale;j++)
					sum += v[(y*scale+i)*w + x*scale+j];
			sum /= (scale*scale);
			*p++ = sum;
		}
	}
	return r;
}
	

long long simple_image_diff( unsigned short *img1 , unsigned short *img2 , int w,int h)
{
	long long diff = 0;
	int i,j;
	for(i=0;i<h;i++) {
		for(j=0;j<w;j++) {
			int v1 = (*img1++);
			int v2 = (*img2++);
			long long d = v1-v2 ;
			diff += d*d;
		}
	}
	return diff;
}


long long cross_correlation( unsigned short *img1 , unsigned short *img2 , int w,int h)
{
	long long sum = w*h*65536LL*65536LL;
	int i,j;
	for(i=0;i<h;i++) {
		for(j=0;j<w;j++) {
			int v1 = (*img1++);
			int v2 = (*img2++);
			sum -= v1*v2;
		}
	}
	return sum;
}


long long fuzzy_image_diff( unsigned short *needle[16][16] , unsigned short *img2 , int w,int h , unsigned char *offsets )
{
	long long diff = 0;
	int i,j;
	int k,l;
	for(i=0;i<h;i++) {
		for(j=0;j<w;j++) {
			long long min = 65536LL*65536LL; 
			unsigned char o_min;
			int v2 = (*img2++);
			for(k=0;k<16;k++)
				for(l=0;l<16;l++) {
					int v1 = needle[k][l][w*i+j];
					long long d = v1-v2 ;
					d = d*d;
					if( d<min ) {
						min = d; 
						o_min = k*16+l;
					}
				}
			diff += min;
			*offsets++ = o_min;
		}
	}
	return diff;
}



static void write_pgm( unsigned short *out_img , int w , int h , char *fname   )
{
	FILE *f;
	int x,y;
	f = fopen(fname,"w");
	fprintf(f,"P5\n%d %d\n%d\n",w,h,65535);
	for(y=0;y<h;y+=1)
		for(x=0;x<w;x+=1) {
			int value =  out_img[y*w+x]  ;
			fputc( value/256 , f);
			fputc( value&255 , f);
		}
	fclose(f);
}


int get_psf3( unsigned short *v , int w , int h , float cx , float cy , int r , float blacklevel , const char *fname   )
{
	int i;	
	int j;
	int k=0;
	int n=2*r+1;
	FILE *f = NULL;
	int max =  get_pixel(v,w,h,cx,cy);
	unsigned short *psf = malloc( (2*r+1)*(2*r+1)*sizeof(short));
	float *sum = malloc( (2*r+1)*(2*r+1)*sizeof(float));
	memset(sum,0,sizeof(float)*(2*r+1)*(2+r+1) );
	
	for( i = -r; i<= r ; i++  ) {
		for( j = -r; j<= r ; j++  ) {
			float val =  get_pixel(v,w,h,cx+j,cy+i);
			if( i == 0 && j == 0 )
				printf("(");
			printf("%6.1f",val);
			if( i == 0 && j == 0 )
				printf(")");
			val -= blacklevel;
			if( val < 0 )
				val = 0;
			printf(" ");
			psf[k++] = val*50000.0/max;
		}
		printf("\n");
	}
#if 0
	for(i=1;i<=r;i++)
		for(j=0;j<=i;j++) {
#define SUM( x , y ) (x) += y
#define ASSIGN( x , y ) (y) = x
			SUM(sum[(r+i)*n+r+j] ,  psf[(r+i)*n+r+j] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r+i)*n+r-j] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r-i)*n+r+j] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r-i)*n+r-j] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r+j)*n+r+i] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r+j)*n+r-i] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r-j)*n+r+i] ); 
			SUM(sum[(r+i)*n+r+j] ,  psf[(r-j)*n+r-i] ); 
		}
	for(i=0;i<=r;i++)
		for(j=0;j<i;j++) {
			sum[(r+i)*n+r+j] /= 8; 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r+i)*n+r+j] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r+i)*n+r-j] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r-i)*n+r+j] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r-i)*n+r-j] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r+j)*n+r+i] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r+j)*n+r-i] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r-j)*n+r+i] ); 
			ASSIGN(sum[(r+i)*n+r+j] ,  psf[(r-j)*n+r-i] ); 
		}
#endif
			
	write_pgm( psf ,2*r+1,2*r+1 , fname );
	free(psf);
}


int get_psf( unsigned short *v , int w , int h , float cx , float cy , int r , float *r__ ,  const char *fname   )
{
	int i;	
	float r_ ;
	float psf[200] = { 0 };
	FILE *f = NULL;
	float min ;
	float max ;
	float fwhm = 0;
	int black_level = 0;
	int j;
	psf[0] = get_pixel(v , w , h, cx , cy );
	for( i = 1; i< 2*r ; i++  ) {
		int count = 0;
		float r_ = 0.5*i;
		float step = 0.5/r_;
		float angle;
		float sum = 0;
		for( angle = 0 ; angle < 2*M_PI ; angle += step ) {
			float x = cx+cos(angle)*r_;
			float y = cy+sin(angle)*r_;
			sum += get_pixel(v,w,h,x,y);
			count++;
		}
		psf[i] = sum/count;
	}
	for( i=0;i<100;i++) {
		printf("%f %f \n",i/2.0 , psf[i] );
	}
	min = psf[0];
	max = min;
	for( i=0;i<100;i++) {
		if( psf[i]<min )
			min = psf[i];
	}
	if( fname )
		f = fopen(fname,"w");
	j = 0;
	for( i=0;i<2*r;i++) {
			float v = psf[i];
			if( v < (max+min)/2 && !fwhm )
				fwhm = i/2.0; 
			if( f )
				fprintf(f,"%f %f\n",i/2.0 , psf[i] );
			black_level = psf[i];
			j = i;
	}
	if( f )
		fclose(f);
	if( r__ )
		*r__ = fwhm;
	return black_level;
}

int get_psf2( unsigned short *v , int w , int h , float cx , float cy , int r , float *r_ , const char *fname   )
{
	int x_ = floor( cx+0.5 );
	int y_ = floor( cy+0.5 );
	int x,y;
	float avg = 0;
	FILE *f = NULL;
	int i;
	float min ;
	float max ;
	float fwhm = 0;
	float psf[200] = { 0 };
	int count[200] = { 0 };
	int black_level = 0;
	int j;
	printf("get_psf %f %f %d\n",cx,cy,r );
	
	for( y = y_ -r ; y <= y_ + r ; y++ ) {
		for( x = x_ -r ; x <= x_ + r ; x++ ) {
			if ( (x-cx)*(x-cx) + (y-cy)*(y-cy) < r*r ) {
				int i;

				float v_ = v[w*y+x];
				float r_ = sqrt( (x-cx)*(x-cx) + (y-cy)*(y-cy) );
				i = floor( 2*r_ + 0.5 );
				psf[i] += v_;
				count[i] ++;
			} 
		}
	}
	for( i=0;i<100;i++) {
		if( count[i] )
			printf("%f %f %d\n",i/2.0 , psf[i]/count[i] , count[i] );
	}
	min = psf[0]/count[0];
	max = min;
	for( i=0;i<100;i++) {
		if( count[i] && psf[i]/count[i]<min )
			min = psf[i]/count[i];
	}
	if( fname )
		f = fopen(fname,"w");
	j = 0;
	for( i=0;i<100;i++) {
		if(  count[i] ) {
			float v = psf[i]/count[i];
			if( v < (max+min)/2 && !fwhm )
				fwhm = i/2.0; 
			if( f && i>0 && !count[i-1] )
				fprintf(f,"%f %f\n",(i-1)/2.0 , ( psf[i]/count[i] + psf[j]/count[j] ) / 2 );
			if( f )
				fprintf(f,"%f %f\n",i/2.0 , psf[i]/count[i] );
			black_level = psf[i]/count[i];
			j = i;
		}
	}
	if( f )
		fclose(f);
	if( r_ )
		*r_ = fwhm;
	return black_level;
}


int get_center_of_light( unsigned short *v , int w , int h , int x_ , int y_ , int r ,  float *cx , float *cy , float black_level , int *count_ )
{
	float sum = 0;
	float sx = 0;
	float sy = 0;
	float sw = 0;
	int x,y;
	int count = 0;
	float avg = 0;
	for( y = y_ -r ; y <= y_ + r ; y++ ) {
		for( x = x_ -r ; x <= x_ + r ; x++ ) {
			if ( (x-x_)*(x-x_) + (y-y_)*(y-y_) < r*r ) {

				float v_ = v[w*y+x];
				// v_ -= black_level;
				if( v_< black_level )
					v_ = 0;
				else
					v_ -= black_level;
	
				sx += v_*v_ * x;
				sy += v_*v_ * y;
				sw += v_*v_;
				avg += v_;
				count++;
			} 
		}
	}
	if(!sw)
		return -1;
	*cx = sx / sw;
	*cy = sy / sw;
	if( count_ )	
		*count_ = count;
	printf("cx = %f cy = %f avg = %f\n",*cx,*cy , avg / count );

	return avg;
}

float get_max_point( unsigned short *v , int w , int h , int x_ , int y_ , int r ,  float *cx , float *cy  )
{
	float sum = 0;
	float sx = 0;
	float sy = 0;
	float sw = 0;
	int x,y;
	int count = 0;
	float max = 0;
	for( y = y_ -r ; y <= y_ + r ; y++ )
		for( x = x_ -r ; x <= x_ + r ; x++ ) {
			if ( (x-x_)*(x-x_) + (y-y_)*(y-y_) < r*r ) {
				float v_ = v[w*y+x];
				if( v_>max ) {
					max = v_;
					*cx = x;
					*cy = y;
				}
			}
		}
	printf("max = %f\n",max );

	return max;
}

