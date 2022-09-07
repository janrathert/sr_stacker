#include <stdio.h>
#include <math.h>
#include "circle.h"
#include "image_match.h"

int max_pixelvalue = 255;

float lambda = 0.1;

#define max_points 1000000
int ox[max_points];
int oy[max_points];
float ox_[10000];
float oy_[10000];
int outline_count = 0;
int outline_count_ = 0;
int outline_idx[65536] = { 0 };
int outline_end[65536] = { 0 };

int min_x = 16;
int min_y = 16;
int max_x = 0;
int max_y = 0;
int min_r = 50;
int max_r = 1000;
float sobel_factor = 1/8.0;
static float threshold = 5000;
static float eps = 30;
static float eps2 = 30;
float degrees = 150;

static int *stack_images[4][4];
static int stack_count[4][4] ;



void copy_2ranges( int begin1,int end1, int begin2, int end2)
{
	int i,j;
	outline_count_ = 0;
	if( begin1 == begin2 )
		end1 = -1;
	for(i=begin1;i!=end2 && outline_count_ < 10000 ;) {
		ox_[outline_count_] = ox[i];
		oy_[outline_count_++] = oy[i];
		
		i++;
		if(i==end1)
			i=begin2;
	}
}

void copy_outlines_by_bitmap( unsigned short *group , int group_len , int bitmap )
{
	int i,j,k;
	outline_count_ = 0;
	int n = 1<<group_len;
	for(i=1,k=0;i<n;i*=2,k++) {
		if( bitmap & i ) {
			for(j=outline_idx[group[k]];j!=outline_end[group[k]];j++) {
				if( outline_count_ >= 10000 )
					return 0;
				ox_[outline_count_] = ox[j];
				oy_[outline_count_++] = oy[j];
			}
		}
	}
	return 0;
}


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

int sobel_x_filter_1[] = {
	3,
	1,0,-1,
	2,0,-2,
	1,0,-1 };

int sobel_y_filter_1[] = {
	3,
	1,2,1,
	0,0,0,
	-1,-2,-1 };

int sobel_x_filter_2[] = {
	6,
	1,1,0,0,-1,-1,
	1,1,0,0,-1,-1,
	2,2,0,0,-2,-2,
	2,2,0,0,-2,-2,
	1,1,0,0,-1,-1,
	1,1,0,0,-1,-1
};

int sobel_y_filter_2[] = {
	6,
	1,1,2,2,1,1,
	1,1,2,2,1,1,
	0,0,0,0,0,0,
	0,0,0,0,0,0,
	-1,-1,-2,-2,-1,-1,
	-1,-1,-2,-2,-1,-1 };


int convol( unsigned short *p, int w, int h, int x , int y , int *filter )
{
	int fw = filter[0];
	int x_,y_;
	int sum = 0;
	if( x < fw || y < fw || x >= w-fw || y >= h-fw )
		return 0;
	x -= fw/2;
	y -= fw/2;
	filter++;
	p += w*y + x;
	for( y_ = 0 ; y_ < fw ; y_++ )  {
		for( x_ = 0 ; x_ < fw ; x_++ )  {
			sum += *filter++ * *p++;
		}
		p += w-fw;
	}
	return sum;
}

#if 0
int sobel_x( unsigned short *p , int w, int h , int x , int y )
{
	int r;
	if( x == 0 || y == 0 || x == w-1 || y == h-1 )
		return 0;
	p += w*y + x;
	return p[-w-1] - p[-w+1] + 2*p[-1] - 2*p[1] + p[w-1] - p[w+1];
}

int sobel_y( unsigned short *p , int w, int h , int x , int y )
{
	int r;
	if( x == 0 || y == 0 || x == w-1 || y == h-1 )
		return 0;
	p += w*y + x;
	return p[-w-1] - p[w-1] + 2*p[-w] - 2*p[w] + p[-w+1] - p[w+1];
}
#endif

float avg33( float *v , int w , int h , int x , int y )
{
	v += w*y+x;
	return v[-1]+v[0]+v[1]
	+ v[-1-w]+v[-w]+v[1-w]
	+ v[-1+w]+v[w]+v[1+w];
}

float avg22( float *v , int w , int h , int x , int y )
{
	v += w*y+x;
	return ( v[0]+v[1]
	    + v[w]+v[1+w] ) / 4;
}

unsigned short getpixel_ushort( unsigned short *v , int w , int h , int x , int y )
{
	if( y < 0 || x < 0 || x >= w || y >= h ) 
		return 0;
	v += w*y+x;
	return *v;
}

float avg( float *v , int w , int h , int x , int y , int n )
{
	int i,j;
	float sum = 0;
	v += w*(y-n)+x-1;
	for(i=0;i<2*n+1;i++) {
		for(j=0;j<2*n+1;j++) {
			sum += *v++;
		}
		v += w - 2*n - 1;
	}
	return sum / (2*n+1)/(2*n+1);
}

int get_center_of_light( float *v , int w , int h , int x_ , int y_ ,  float *cx , float *cy )
{
	float sum = 0;
	float sx = 0;
	float sy = 0;
	float sw = 0;
	int x,y;
	for( y = y_ -1 ; y <= y_ + 1 ; y++ )
		for( x = x_ -1 ; x <= x_ + 1 ; x++ ) {
			sx += v[w*y+x] * (x+0.5);
			sy += v[w*y+x] * (y+0.5);
			sw += v[w*y+x];
		}
	if(!sw)
		return -1;
	*cx = sx / sw;
	*cy = sy / sw;

	return 0;
}

float avg_gradient( int *sx , int *sy , int w, int h , int x, int y, int w_ , int h_ , float *gx , float *gy )
{
	float weight = 0;
	float sumx = 0;
	float sumy = 0;
	float l;
	int x2,y2;
	float x0 = x + w_/2;
	float y0 = y + h_/2;
	for(y2 = y ; y2 < y + h_ ; y2++ ) {
		for(x2 = x ; x2 < x + w_ ; x2++ ) {
			long long  sx_ = sx[y2*w+x2];
			long long  sy_ = sy[y2*w+x2];
			float r = sqrt( (x2-x0)*(x2-x0) + (y2-y0)*(y2-y0) ) ;
			float weight_ = sqrt( sx_*sx_ + sy_*sy_ );
			if( r > w_/2 + 1 )
				weight_ = 0;
			if( r > w_/2  )
				weight_ *= ( w_/2 +1 -r );  
			weight += weight_;
			sumx += weight_ * sx_;
			sumy += weight_ * sy_;
		}
	}
	if(!weight ) {
		printf("zero avg !!!\n");
		return;
	}
	*gx = sumx ;
 	*gy = sumy ;
	l = sqrt( *gx * *gx + *gy * *gy );
	*gx /= l;
	*gy /= l;
	return l;

}

float linear_edge( int *sx , int *sy , int w, int h , int x, int y, int w_ , int h_ , float gx , float gy , float *bad_ , float threshold )
{
	float weight = 0;
	float sumz = 0;
	float l;
	int x2,y2;
	float gx_ = -gy;
	float gy_ = gx ;
	float x0 = x + w_/2;
	float y0 = y + h_/2;
	float d_ = w_/2 * 0.7;
	FILE *f = NULL;
	static int i = 0;
	int bad = 0;
	char fname[1024];
	if( !bad_ ) {
		sprintf(fname,"edge%d.pgm",i++);
		f = fopen(fname,"w");
		fprintf(f,"P5\n%d %d\n%d\n",w_,h_,max_pixelvalue);
	}

	for(y2 = y ; y2 < y + h_ ; y2++ ) {
		for(x2 = x ; x2 < x + w_ ; x2++ ) {
			int sx_ = sx[y2*w+x2];
			int sy_ = sy[y2*w+x2];
			float d;
			float factor=1;
			float weight_ = sqrt( sx_*(long long)sx_ + sy_*(long long)sy_ );
			float z = x2 * gx + y2 * gy;
			d = fabs( (x2-x0)*gx_ + (y2-y0)*gy_ );
			if( d > d_ + 1 )
				factor = 0;
			else if( d > d_ )
				factor *= ( d_ +1 -d );  
			d = fabs( (x2-x0)*gx + (y2-y0)*gy ); 
			if( d > d_ + 1 )
				factor = 0;
			else if( d > d_ )
				factor *= ( d_ +1 -d );  
			if( weight_ < threshold )
				weight_ = 0;
			
			if( bad_ ) {
				if( d > d_ *0.3  && weight_ * factor > *bad_ ) {
					bad = 1;
				}
			}
			weight_ *= factor;
			if( f )  {
				if( factor < 1 ) {
					fputc( (1-factor)*255  , f);
					fputc( 0  , f);
				} else {
					int v = weight_;
					fputc( v/256  , f);
					fputc( v&255  , f);
				}
			}
			weight += weight_;
			sumz += z * weight_;
		}
	}
	if( f ) 
		fclose(f);
	if(!weight ) {
		printf("zero le !!!\n");
		return 0;
	}
	if( bad_ )
		*bad_ = bad;
	else {
		printf("return sumz = %f weight = %f %f\n",sumz,weight ,sumz / weight );
	}
	return sumz / weight;

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


int count_neighbours(  unsigned short *m , int w , int h , int x, int y , int n , int colour )
{
	int x2,y2;
	int i,j;
	int count = 0;
	for(i= -n;i<=n;i++)
		for(j= -n;j<=n;j++) {
			y2 = y + i; 
			x2 = x + j; 
			if( (i||j) && m[y2*w+x2] == colour  )
				count++; 
		}
	return count;
}

int stop_x;
int stop_y;
int stop_x1;
int stop_y1;
int stop_x2;
int stop_y2;

int count_neighbours_with_different_colour(  unsigned short *m , int w , int h , int x, int y , int n , int colour )
{
	int x2,y2;
	int i,j;
	int count = 0;
	for(i= -n;i<=n;i++)
		for(j= -n;j<=n;j++) {
			y2 = y + i; 
			x2 = x + j; 
			if( (i||j) &&  m[y2*w+x2] && m[y2*w+x2] != colour  ) {
				stop_x = x2;
				stop_y = y2;
				count++; 
			}
		}
	return count;
}


float find_max( float *v , int w, int h , int *x, int *y , int cx , int cy , int r  )
{
	float max = 0;
	int x_,y_;
	for(y_=0;y_<h;y_++) 
		for(x_=0;x_<w;x_++)  {
			if ( (x_ - cx ) * (x_ - cx ) + (y_ - cy) * (y_ - cy) > r*r )
				continue;
			if( v[y_*w+x_] > max ) {
				max = v[y_*w+x_];
				*x = x_;
				*y = y_;
			}
		}
	return max;
}


int mark_neighbour( float *s , unsigned short *m , int w , int h , int *x_, int *y_ )
{
	int i,j;
	float max = 0;
	int i_max,j_max;
	int x2,y2;
	int x = *x_;
	int y = *y_;
	int n = 4;
	int n_ = 6;
	int colour = m[y*w+x];
	if(*x_ < min_x || *y_<min_y || *x_>max_x || *y_>max_y )
		return -1;
	for(i= -1;i<2;i++)
		for(j= -1;j<2;j++) {
			y2 = y + i; 
			x2 = x + j; 
			if( ( i!=0 || j!=0 ) && !m[y2*w+x2] && s[y2*w+x2] > max
				&& count_neighbours(m,w,h,x2,y2 ,1 , colour ) <= 2 
				&& count_neighbours(m,w,h,x2,y2 ,n , colour ) <= n_  ) {
				*x_ = x2;
				*y_ = y2;
				max =  s[y2*w+x2] ;
			}
		}
	if ( max == 0 )
		return -1;
	if( m[ (*y_)*w+*x_] )
		return -2; 
	m[ (*y_)*w+*x_] = m[y*w+x]   ;
	return 0;

}

int recolour_neighbour(  unsigned short *m , int w , int h , int *x_, int *y_ , int old_colour, int new_colour )
{
	int i,j;
	float max = 0;
	int i_max,j_max;
	int x2,y2;
	int x = *x_;
	int y = *y_;
	int n = 4;
	int n_ = 6;
	if(*x_ < 16 || *y_<16 || *x_>w-16 || *y_>h-16 )
		return -1;
	for(i= -1;i<2;i++)
		for(j= -1;j<2;j++) {
			y2 = y + i; 
			x2 = x + j; 
			if( ( i!=0 || j!=0 ) && m[y2*w+x2] == old_colour ) {
				m[ y2*w+x2] = new_colour   ;
				*x_ = x2;
				*y_ = y2;
				return 0;
			}
		}
	return -1;
}


float outline_error( float cx, float cy , float r )
{
	float err = 0;
	int i;
	for(i=0;i<outline_count_;i++) {
		float diff = sqrt( ( ox_[i] - cx ) * ( ox_[i]-cx ) + ( oy_[i] -cy ) * ( oy_[i] - cy ) ) - r;
		diff = diff * diff;
		err += diff;
	}
	return err;
}

void remove_points_too_far( float cx, float cy , float r , float reldist )
{
	float err = 0;
	int i;
	int j;
	reldist *= r;
	for(i=0,j=0;i<outline_count_;i++) {
		float diff = sqrt( ( ox_[i] - cx ) * ( ox_[i]-cx ) + ( oy_[i] -cy ) * ( oy_[i] - cy ) ) - r;
		if( diff > -reldist && diff < reldist ) {
			ox_[j] = ox_[i];
			oy_[j] = oy_[i];
			j++;
		}
	}
	printf("%d points deleted\n",outline_count_ - j );
	outline_count_ = j;
}



void find_best_center( float cx1, float cx2 , float cy1 , float cy2 , float r1 , float r2 , float *cx , float *cy , float *r , int steps ,
			int r_steps  )
{
	float min = 1e30;
	float cx_,cy_;
	float r_;
	int i,j,k;
	float x_delta = (cx2-cx1)/steps;
	float y_delta = (cy2-cy1)/steps;
	float r_delta = (r2-r1)/r_steps; 
	printf("x range %f %f\n",cx1,cx2);
	printf("y range %f %f\n",cy1,cy2);
	printf("r range %f %f\n",r1,r2);
	for( r_ = r1,i=0 ; i < r_steps ; r_ += r_delta , i++ )
		for( cx_ = cx1, j = 0 ; j<steps ; cx_ += x_delta , j++ )
			for( cy_ = cy1 , k=0 ; k < steps ; cy_ += y_delta , k++  ) {
				float err = outline_error( cx_,cy_,r_ );
				if( err < min ) {
					*cx = cx_;
					*cy = cy_;
					*r = r_;
					min = err ;
				}
			}

}

void unmark_outline(unsigned short *m,int w,int begin,int end,int val)
{
	int i;
	for(i=begin;i<end;i++) {
		m[oy[i]*w+ox[i]] = val;
	}
}

int check_outline(unsigned short *m,int w,int h,int begin,int end,int val)
{
	int i;
	int x,y;
	int cnt = end-begin;
	int cnt2 = 0;
	for(i=begin;i<end;i++) {
		if( m[oy[i]*w+ox[i]] != val ) {
			printf("check failed colour=%d is=%d begin=%d end=%d i=%d\n",val, m[oy[i]*w+ox[i]] , begin,end,i);
			exit(1);

		} 
	}
	for(y=0;y<h;y++)
		for(x=0;x<w;x++)
			if( m[y*w+x] == val )
				cnt2++;
	if( cnt != cnt2 ) {
		printf("outline check 2 failed colour=%d %d %d\n",val,cnt,cnt2);
		exit(1);
	}
}

void set_outline_colour(unsigned short *m,int w,int val, int count)
{
	int i;
	for(i=0;i<count;i++) {
		m[oy[i]*w+ox[i]] = val;
	}
}

int recolour_outline( unsigned short *m , int w, int h, int sx,int sy, int colour )
{
	int x = sx;
	int y = sy;
	int old_colour = m[y*w+x];
	int i;
	int k= -1;
	int j=0;
	int found = 0;
	printf("recolour %d %d %d\n",old_colour , outline_idx[old_colour] , outline_end[old_colour ] );
	for( i = outline_idx[old_colour] ; i!=outline_end[old_colour] ; i++  ) {
		if( ox[i] == sx && oy[i] == sy ) {
			outline_idx[colour] = i;
			k = i;
			found = 1;
		}
		if( found ) {
			m[ oy[i]*w + ox[i] ] = colour;
		}
	}
	if( found ) {
		outline_idx[colour] = k; 
		outline_end[colour] = i;
		outline_end[old_colour] = k;
	}
	return 0;
}



int mark_outline( float *s , unsigned short *m , int w , int h , int sx, int sy , float *cx_ , float *cy_ , float *r_ , int colour )
{
	int i=0;
	int j;
	int j_;
	int x = sx,y = sy;	
	int ret = 0;
	int begin = 0;
	float max = 0;
	float val_;
	float div = 20;
	int outline_begin;

	float x1,y1;
	if( m[y*w+x] )
		return -1; 
	m[y*w+x] = colour;
	outline_begin = outline_count;
	outline_idx[colour] = outline_begin;
	ox[outline_count] = sx;
	oy[outline_count++] = sy;
	stop_x = -1;
	stop_y = -1;
	stop_x1 = -1;
	stop_y1 = -1;
	stop_x2 = -1;
	stop_y2 = -1;
	// printf("mark_outline %d %d\n",sx,sy);
	val_ = s[y*w+x];
	while( (ret=mark_neighbour( s,m, w,h, &x,&y )) == 0 && s[y*w+x] >= threshold && outline_count < max_points  ) {
		if( s[y*w+x] > max ) {
			max = s[y*w+x];
			// threshold = max/div;
		}
		if( s[y*w+x] < val_ && !begin ) {
			unmark_outline(m,w,outline_begin,outline_count,0);
			outline_count = outline_begin;
			sx = x;
			sy = y;
			begin = 1; 
		}
		val_ = s[y*w+x];
		ox[outline_count] = x;
		oy[outline_count++] = y;
		i++;
		// check_outline(m,w,h,outline_begin,outline_count , colour );
		if( count_neighbours_with_different_colour(m,w,h,x,y ,1 , colour ) ) {
			ret = -1;
			stop_x1 = stop_x;
			stop_y1 = stop_y;
			break;   
		}
		if( outline_count - outline_begin == 50 )  {
			unmark_outline(m,w,outline_begin,outline_begin+25,colour+1);
		}
	}
	unmark_outline(m,w,outline_begin,outline_count,colour);
	if( ret == 0 )
		m[y*w+x] = 0;
	if( stop_x1 != -1 ) {
		// printf("stop_x1 = %d stop_y1 = %d sx %d sy %d\n",stop_x1,stop_y1 , sx,sy  );
		if( stop_x1 == sx && stop_y1 == sy ) {
			printf("loop\n");
			stop_x1 = -1;
			stop_y1 = -1;
			outline_end[colour] = outline_count ;
			return -1;
		}
	}
	for(j=outline_begin,j_=outline_count-1;j<(outline_begin+outline_count)/2;j++,j_--) {
		int tmp = ox[j];
		ox[j] = ox[j_];
		ox[j_] = tmp;
		tmp = oy[j];
		oy[j] = oy[j_];
		oy[j_] = tmp;
	}
	x1 = x;
	y1 = y;
	
	x=sx;
	y=sy;
	// set_outline_colour(m,w,colour+1,outline_count/2);
	if( outline_count - outline_begin >= 50 ) 
		unmark_outline(m,w,outline_begin,outline_begin+25,colour+1);
	while( (ret=mark_neighbour( s,m, w,h, &x,&y )) == 0 && s[y*w+x] >= threshold && outline_count < max_points  ) {
		if( s[y*w+x] > max ) {
			max = s[y*w+x];
			// threshold = max/div;
		}
		ox[outline_count] = x;
		oy[outline_count++] = y;
		// check_outline(m,w,h,outline_begin,outline_count , colour );
		i++;
		if( count_neighbours_with_different_colour(m,w,h,x,y ,1 , colour ) ) {
			ret = -1;
			stop_x2 = stop_x;
			stop_y2 = stop_y;
			break;   
		}
	}
	if( ret == 0 )
		m[y*w+x] = 0;
	unmark_outline(m,w,outline_begin,outline_count,colour);
	outline_begin = outline_idx[colour];
	outline_end[colour] = outline_begin ;
	if(outline_count == outline_begin)
		return -1;
	if( i<20 ) {
		unmark_outline(m,w,outline_begin,outline_count,0);
		outline_count = outline_begin;
		return -1;
	}
	outline_end[colour] = outline_count ;
	// check_outline(m,w,h,outline_begin,outline_count , colour );
	
	return -1;
}

void change_group( unsigned short *groups , int old , int new )
{
	int i;
	for(i=0;i<65536;i++)
		if( groups[i] == old )
			groups[i] = new;
}

int mark_all_outlines( float *s , unsigned short *m , int w , int h , int sx, int sy , float *cx_ , float *cy_ , float *r_ )
{
	static float r;
	static float cx;
	static float cy;
	static float r_range = 5;
	static int r_steps = 40;
	float s_old=0;
	int rising=0;
	unsigned short group[655356];
	float error;
	int x,y;
	int i,j;
	int i1,i2;
	int g;
	int found=0;
	unsigned short colour = 1;
	memset( group, 0, sizeof(group) );
	for(y=min_y;y<max_y;y++)
		for(x=min_x;x<max_x;x++) {
			int ret;
			if(!m[y*w+x] && s[y*w+x] > threshold  ) {
				int c = colour;
				ret = mark_outline(s,m,w,h,x,y,cx_,cy_,r_,colour);
				if( outline_end[colour] != outline_idx[colour] ) {
					group[c] = c ;
					printf("outline_count %d = %d \n",c,outline_count);
					// group[c+1] = c ;
					colour+=1;
					if( stop_x1 != -1 ) {
					    change_group( group, group[ m[stop_y1*w+stop_x1] ] , c );
					    group[ m[stop_y1*w+stop_x1] ] = c ;
					    group[ colour ] = c;
					    recolour_outline( m , w, h, stop_x1,stop_y1 ,  colour );
					    // Nm[stop_y1*w+stop_x1]=0xffff;
					    colour++;
					}
					if( stop_x2 != -1 ) {
					    change_group( group, group[ m[stop_y2*w+stop_x2] ] , c );
					    group[ m[stop_y2*w+stop_x2] ] = c ;
					    group[ colour ] = c;
					    recolour_outline( m , w, h, stop_x2,stop_y2 ,  colour );
					    // m[stop_y2*w+stop_x2]=0xffff;
					    colour++;
					}
					if(!colour) {
						printf("too many colours\n");
						exit(1);
					}	
					printf("colour = %d\n",colour);
				}
				if( ret == 0 )
					return 0;
			}
		}
	write_pgm_s(m,w,h,"outline.pgm",0);
        printf("colour=%d outline_count = %d\n",colour , outline_count);
	for(g=1;g<colour;g++)
		printf("group %d %d\n",g,group[g]);
	for(g=1;g<colour && !found;g++) {
		int k=0;
		unsigned short map[65536];
		int n;
		int i_,j_;
		for(i=1;i<colour;i++)
			if( group[i] == g ) {
				copy_2ranges(outline_idx[i],outline_end[i],outline_idx[i],outline_end[i]);
				// if(fit_circle(ox_,oy_,outline_count_,&cx,&cy,&r) == 0 && r > min_r  ) {
					map[k++] = i;
				/*
				} else {
					int i;
					for(i=0;i<outline_count_;i++) 
						m[((int)oy_[i])*w+((int)ox_[i])] = 0;
				}
				*/
			}
		n=k;
		if(!n)
			continue;
		printf("group %d n=%d\n",g,n);
		if(n>20)
			continue;
		if( n > 4000 ) {
			for(y=0;y<h;y++)
				for(x=0;x<w;x++)
					if( group[ m[y*w+x] ] != g && m[y*w+x] )
						m[y*w+x] = 65535;
			write_pgm_s(m,w,h,"group.pgm",0);
			exit(1);
		}
			
		
		for(i_=0;i_<n && !found;i_++)
			for(j_=0;j_<=i_ && !found;j_++) {
				int i1,i2;
				float err_ratio = 0;
				i = map[i_];
				j = map[j_];
				if( outline_end[i]-outline_idx[i] > min_r && outline_end[j] - outline_idx[j] > min_r ) {
					// check_outline(m,w,h,outline_idx[i],outline_end[i],i);
					copy_2ranges(outline_idx[i],outline_end[i],outline_idx[j],outline_end[j]);
					if( fit_circle(ox_,oy_,outline_count_,&cx,&cy,&r) < 0 ) {
						outline_count_ = 0;
						printf("fit circle failed\n");
					}
				}  else
					outline_count_ = 0;
					
				if( outline_count_ == 0 || outline_count_ < r || !( r > min_r && r <  max_r ) || ( err_ratio = outline_error(cx,cy,r) / outline_count_ ) > (r/eps)*(r/eps)  ) {
					// printf("no circle %d  cx %f cy %f r %f\n",outline_count_,cx,cy,r);
					printf("err_ratio = %f count = %d\n",err_ratio , outline_count_ );
					if(r_range)
						r = 0;
				} else {
					printf("i1=%d %d %d i2=%d  %d %d\n",i1, ox[i1],oy[i1] , i2, ox[i2],oy[i2] );
					printf("%d %d %d %d\n",outline_idx[i],outline_end[i],outline_idx[j],outline_end[j] );
					printf("circle %d %d %d\n",outline_count,i,j);
					found = 1;
				}

			}
		if(found) {
			int bitmap;
			printf("n = %d\n",n);
			if( n < 21 ) {
				for(bitmap=1;bitmap<1<<n;bitmap++) {
					int ret ;
					float cx_,cy_,r_;
					float error = 0;
					copy_outlines_by_bitmap(map,n,bitmap);
					ret = fit_circle(ox_,oy_,outline_count_,&cx,&cy,&r);
					if( ret == 0 && r>min_r && r<max_r && outline_count_ > 0.99*sqrt(2)*degrees/90*r && 
						( error = outline_error(cx,cy,r) / outline_count_ ) < (r/eps2)*(r/eps2)  ) {
						found=2;
						
						printf("found bitmap=%x error = %f\n",bitmap , error);
						printf("count %d\n",outline_count_ );
						break;
					} else {
						printf("error = %f ret = %d r = %f count %d\n",error,ret,r,outline_count_);

					}
				}
			}

		}
		if( found == 1 )
			found = 0;
	}
	// cx += 0.5;
	// cy += 0.5;
	printf("cx = %f cy = %f r = %f found = %d\n",cx,cy,r,found);
	if(found!=2) {
		outline_count_ = 0;
		return -1;
	}
	printf("outline_count=%d after add to circle\n",outline_count);
	printf("cx %f cy %f r %f\n",cx,cy,r);
	if( r_range ) {
		max_r = 1.01 * r;
		min_r = 0.99 * r;
	}
	m[((int)cy)*w+(int)cx] = 32767;
	j = 0;
	remove_points_too_far(cx,cy,r,0.1);
	fit_circle(ox_,oy_,outline_count_,&cx,&cy,&r);
	remove_points_too_far(cx,cy,r,0.05);
	fit_circle(ox_,oy_,outline_count_,&cx,&cy,&r);
	remove_points_too_far(cx,cy,r,0.025);
	for(i=0;i<outline_count_;i++) {
		m[((int)oy_[i])*w+((int)ox_[i])] = 32767;
		get_center_of_light( s , w , h , ox_[i] , oy_[i] , &ox_[i] , &oy_[i] );
	}
	fit_circle(ox_,oy_,outline_count_,&cx,&cy,&r);
	error = outline_error(cx,cy,r) / outline_count_;
	// cx += 0.5;
	// cy += 0.5;
	printf("cx %f cy %f r %f after remove points error = %f\n",cx,cy,r , error );
	if(!r) {
		outline_count_ = 0;
		return -1;
	}
	
	*cx_ = cx;
	*cy_ = cy;
	*r_ = r;
	/*
	min_x = cx - 3*r;
	max_x = cx + 3*r;
	min_y = cy - 3*r;
	max_y = cy + 3*r;
	*/
	// eps = 15;
	r_range = 0;
	sleep(5);
	return 0;
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
	unsigned short *first_img;
	int rel_needle_x = 60;
	int rel_needle_y = 60 ;
	int needle_x = 0;
	int needle_y = 0;
	
	float *out_img;
	int zoom=1;
	char out_fname[1024];
	int bad[100];
	dx[0] = 0;
	dy[0] = 0;


	while( argv[1][0] == '-' ) {	
		if( strcmp(argv[1] , "-zoom" ) == 0 ) {
			argv++;
			argc--;

			zoom = atoi(argv[1] );
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-window" ) == 0 ) {
			argv++;
			argc--;

			min_x = atoi(argv[1] );
			argv++;
			argc--;

			min_y = atoi(argv[1] );
			argv++;
			argc--;

			max_x = atoi(argv[1] );
			argv++;
			argc--;

			max_y = atoi(argv[1] );
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-min_r" ) == 0 ) {
			argv++;
			argc--;

			min_r = atoi(argv[1] );
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-max_r" ) == 0 ) {
			argv++;
			argc--;

			max_r = atoi(argv[1] );
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-threshold" ) == 0 ) {
			argv++;
			argc--;

			threshold = atoi(argv[1] );
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-sobel_factor" ) == 0 ) {
			argv++;
			argc--;

			sscanf(argv[1],"%f",&sobel_factor);
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-degrees" ) == 0 ) {
			argv++;
			argc--;

			degrees = atoi(argv[1] );
			argv++;
			argc--;
		} else if( strcmp(argv[1] , "-eps" ) == 0 ) {
			argv++;
			argc--;

			eps = atoi(argv[1] );
			argv++;
			argc--;
			eps2 = atoi(argv[1] );
			argv++;
			argc--;
		} else {
			printf("invalid arg %s\n",argv[1]);
			exit(1);
		}
	}
			
		
	for(i = 1 ; i< argc ;i++ ) {
		int ret;
		float cx,cy,r;
		unsigned short *p = read_pgm(&w,&h,argv[i] );
		if( i== 1 )
			first_img = p;
		printf("w = %d h = %d\n",w,h);
		int *sx_ = malloc( w*h*sizeof(int) );
		int *sy_ = malloc( w*h*sizeof(int) );
		float *s_ = malloc( w*h*sizeof(float) );
		float *s2 = malloc( w*h*sizeof(float) );
		float *mark_ = malloc( w*h*sizeof(unsigned short) );
		memset( mark_ , 0 , w*h * sizeof(unsigned short) );
		outline_count = 0;
		printf("-------------------------------- i=%d %s\n",i,argv[i]);
		for(y=16;y<h-16;y++) {
			for(x=16;x<w-16;x++) {
				long long sx,sy;
				float s;
				sx = convol(p,w,h,x,y,sobel_x_filter_2);
				sy = convol(p,w,h,x,y,sobel_y_filter_2);
				sx_[y*w+x] = sx;
				sy_[y*w+x] = sy;
				s = sqrt(sx*sx + sy*sy )*sobel_factor;
				s_[y*w+x] = s;
				if( i==1 ) {
					if( s > max ) {
						x_max = x;
						y_max = y;
						max = s;
					}
				}
			}
		}
		for(y=16;y<h-16;y++) {
			for(x=16;x<w-16;x++) {
				s2[y*w+x] = avg(s_,w,h,x,y,1);
			}
		}
		find_max(s_,w,h,&x_max,&y_max,w/2,h/2,w); 
		write_pgm(s_,w,h,"sobel.pgm",0);
		write_pgm(s_,w,h,"sobel_inv.pgm",1);
		write_pgm(s2,w,h,"sobel2.pgm",0);
		bad[i] = mark_all_outlines(s_,mark_,w,h,x_max,y_max , &cx,&cy,&r );
		if( !bad[i] )
		{
			char fname_match[256];
			sprintf(fname_match,"match%04d.pgm",i);
			if( i== 1 ) {
				rel_needle_x = 1080 - cx;
				rel_needle_y = 910 - cy;
				
				needle_x = cx + rel_needle_x;
				needle_y = cy + rel_needle_y;
				write_pgm_ushort(p,w,h,fname_match , needle_x-16 , needle_y-16 , 64 ,64 );
				
			} else {
				long diff;
				int xmatch;
				int ymatch;
				int search_x = cx + rel_needle_x;
				int search_y = cy + rel_needle_y;
				diff = best_image_match( first_img , p , w , h ,w ,h , needle_x , needle_y , search_x , search_y , 32 , 32 , &xmatch, &ymatch );
				printf("match x %d y %d\n",xmatch - search_x , ymatch - search_y,y);
				write_pgm_ushort(p,w,h,fname_match , xmatch-16 , ymatch-16 , 64 ,64 );
				char fname_match[256];
				sprintf(fname_match,"search%04d.pgm",i);
				write_pgm_ushort(p,w,h,fname_match , search_x-16 , search_y-16 , 64 ,64 );

			}

		}
		write_pgm_s(mark_,w,h,"outline%d_c.pgm",i);
		if(i==1) {
			cx0 = cx;
			cy0 = cy;
		}
		dx[i] = cx-cx0;
		dy[i] = cy-cy0;
		// find_max(s_,w,h,&x_max,&y_max,640,740,r/20); 
		// mark_outline(s_,mark_,w,h,x_max,y_max , &cx,&cy,&r );
		// write_pgm(mark_,w,h,"outline2.pgm");
		

		printf("i=%d dx = %f dy = %f\n",i,dx[i],dy[i]);
		if( i != 1 )
			free(p);
		free(sx_);
		free(sy_);
	}
	argc = i;
	for( y=0; y<zoom;y++)
		for( x=0; x<zoom;x++) {
			stack_images[y][x] = malloc( w*h*sizeof(int) );
			memset( stack_images[y][x] , 0 , w*h*sizeof(int) );
		}
	for( i=1; i<argc ; i++ ) {
		int w,h;
		int dxi,dyi;
		int dxi_,dyi_;
		if( bad[i] )
			continue;
		dxi = 1024 + dx[i]*zoom + 0.5;
		dyi = 1024 + dy[i]*zoom + 0.5;
		dxi_ = dxi%zoom;
		dyi_ = dyi%zoom;
		dxi = dx[i];
		dyi = dy[i];
		
		printf("i = %d dxi_ = %d dyi_ = %d dxi=%d dyi=%d %s\n",i,dxi_,dyi_ , dxi,dyi , argv[i] );
		unsigned short *p = read_pgm(&w,&h,argv[i] );
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

