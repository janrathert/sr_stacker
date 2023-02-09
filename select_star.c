
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "image_match.h"
#include "part4_star_centroid.h"
typedef unsigned short valtype;
#include "normalize.h"
#include "otsu.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include "track_star.h"
int black_level = 0;
int cross_power_factor = 3;

int max_pixelvalue = 255;
int max_x = 0;
int max_y = 0;
float div_=256;
int cmp_size = 32;
int x_offset;
int y_offset;

int no_ui = 0;
static char ref_image_name_[1024];
static char *ref_image_name;
static char *register_in_name;
static SDL_Window* window;
static SDL_Surface *surface;
#define CROSS_SPECTRUM

unsigned short getpixel_ushort( unsigned short *v , int w , int h , int x , int y )
{
	if( y < 0 || x < 0 || x >= w || y >= h ) 
		return 0;
	v += w*y+x;
	return *v;
}

unsigned int getpixel_uint( unsigned int *v , int w , int h , int x , int y )
{
	if( y < 0 || x < 0 || x >= w || y >= h ) 
		return 0;
	v += w*y+x;
	return *v;
}


unsigned short* read_pgm(  int *w , int *h,  const char *fname , int *x_offset , int *y_offset , int black_level )
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
	if(x_offset && y_offset) {
		*x_offset = 0;
		*y_offset = 0;
	}
	if( strcmp(fname+strlen(fname)-4,".pgm") != 0 ) {
		char cmd[1024];
		sprintf(cmd,"convert -channel green -geometry 1560x2080  %s pgm:-",fname);
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
		if(x_offset && y_offset && strncmp(line,"#offset ",8) == 0 ) {
				sscanf(line+8,"%d %d",x_offset,y_offset);
		}
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
			v = ( p1*256 + p2 )  - black_level ;
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


/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
	SDL_Quit();
	exit(rc);
}

static Uint8 *buffer;

int init_screen( int w , int h )
{
	Uint8  video_bpp;
	Uint32 videoflags;
	int i, k, done;
	SDL_Event event;
	Uint16 *buffer16;
        Uint16 color;
        Uint8  gradient;
	int x;
	int y;


	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		return(1);
	}

	video_bpp = 0;
	videoflags = SDL_SWSURFACE;
	// SDL_putenv("SDL_VIDEO_WINDOW_POS=1600,500");
	window = SDL_CreateWindow("Hello World!", 100, 100, w, h, SDL_WINDOW_SHOWN);
	printf("window = %p\n",window );
	surface = SDL_GetWindowSurface( window );
	printf("surface  = %p\n",surface );

#if 0
	{
SDL_SysWMinfo info;
Window root;
Window parent;
Window *children;
unsigned int children_count;
// SDL_VERSION(&info.version);

if (SDL_GetWMInfo(&info) > 0 ) {
	if (info.subsystem == SDL_SYSWM_X11) {
		XQueryTree(	info.info.x11.display,
				info.info.x11.window,
				&root,
				&parent,
				&children,
				&children_count);
		info.info.x11.lock_func();
		XMoveWindow(	info.info.x11.display,
				parent,
				x,
				y);
		info.info.x11.unlock_func();
		// if( children ) XFree(children);
	}
}

}
#endif

}
static float x_cursor[4] = {-1,-1,-1,-1} ;
static float y_cursor[4] ;
static int method[4]={1,1,1,1};
static int cursor_count = 0;



static inline void set_pixel(int x,int y, int c)
{
	unsigned char *buf = (Uint8 *) surface->pixels;
	buf += surface->pitch *y + 4*x;
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	if( (x^y) & 1 )
		buf[c] = 255;

}


void mark_cursor( int x, int y , int c ) 
{
	int i;
	int r=cmp_size/2;
	printf("mark_cursor %d %d %d\n",x,y,c);
	for(i=0;i<=r;i++) {
		set_pixel( x - i , y - r , c );
		set_pixel( x + i , y - r , c );
		set_pixel( x - i , y + r , c );
		set_pixel( x + i , y + r , c );
		set_pixel( x - r , y - i , c );
		set_pixel( x - r , y + i , c );
		set_pixel( x + r , y - i , c );
		set_pixel( x + r , y + i , c );
	}
}



void draw_cursors()
{
	int i;
	for(i=0;i<cursor_count;i++) {
		if(x_cursor[i]>=0 )
			mark_cursor( x_cursor[i], y_cursor[i] , (i%3) );
	}

}


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

void remove_gradient( unsigned short *img , int w, int h, int X , int Y )
{
	unsigned long long circular_sum[5000];
	int circular_count[5000];
	int i;
	int x,y;
	unsigned short *p;
	h -= 10;
	for( i = 0 ; i< 5000 ; i++ ) {
		circular_count[i] = 0;
		circular_sum[i] = 0;
	}
	for( y = 0 , p=img ; y<h ; y++ ) 
		for( x = 0 ; x<w ; x++ , p++ ) {
			int d = sqrt ( (x-X)*(x-X) + (y-Y)*(y-Y) );
			circular_count[d]++;
			circular_sum[d]+= *p;
			
		}
	for( i = 0 ; i< 5000 ; i++ ) {
		if( circular_count[i] )
			circular_sum[i] /= circular_count[i];
	}

	for( y = 0 , p=img ; y<h ; y++ ) 
		for( x = 0 ; x<w ; x++ , p++ ) {
			int d = sqrt ( (x-X)*(x-X) + (y-Y)*(y-Y) );
			// *p = circular_sum[d];
			if( *p > circular_sum[d] )
				*p -= circular_sum[d];
			else
				*p = 0;
			
		}

}


void showpic_zoom( unsigned short *img , int w, int h , int x , int y , int zoom  )
{
	int i,j;

	/* Set the surface pixels and refresh! */
	if ( SDL_LockSurface(surface) < 0 ) {
		fprintf(stderr, "Couldn't lock the display surface: %s\n",
							SDL_GetError());
		quit(2);
	}
	buffer=(Uint8 *)surface->pixels;
		
        	for ( i=0; i<surface->h; ++i ) {
			int j;
			for(j=0;j<surface->w;j++) {
				int val = get_pixel(img,w,h, (x+j)/(float)zoom , (y+i)/(float)zoom)/div_;
				if( val > 65535 )	
					val = 65535;
				buffer[4*j]=val;
				buffer[4*j+1]=val;
				buffer[4*j+2]=val;
				buffer[4*j+3]=255;
			}
        		// memset(buffer,(i*255)/surface->h, screen->pitch);
        		buffer += surface->pitch;
        	}
	SDL_UnlockSurface(surface);
	SDL_UpdateWindowSurface(window);

}

void showpic( unsigned short *img , int w , int h   )
{
	int i,j;

	/* Set the surface pixels and refresh! */
	if ( SDL_LockSurface(surface) < 0 ) {
		fprintf(stderr, "Couldn't lock the display surface: %s\n",
							SDL_GetError());
		quit(2);
	}
	buffer=(Uint8 *)surface->pixels;
		
        	for ( i=0; i<surface->h; ++i ) {
			int j;
			for(j=0;j<surface->w;j++) {
				int val = img[i*w+j]/div_;
				if( val > 255 )
					val = 255;
				buffer[4*j]=val;
				buffer[4*j+1]=val;
				buffer[4*j+2]=val;
				buffer[4*j+3]=255;
			}
        		// memset(buffer,(i*255)/screen->h, screen->pitch);
        		buffer += surface->pitch;
        	}
	draw_cursors();
	SDL_UnlockSurface(surface);

	SDL_UpdateWindowSurface(window);
}

long long get_img_avg( unsigned short *p , int w, int h , int x_ , int y_, int w_ , int h_ )
{
	long long sum = 0;
	int x,y;
	if(!w_ || !h_)
		return 0;
	for(y=y_;y<y_+h_;y++)
		for(x=x_;x<x_+w_;x++)
			sum += getpixel_ushort( p , w,h, x,y );
	
	return sum / (w_*h_);
}

int get_img_max( unsigned short *p , int w, int h , int x_ , int y_, int w_ , int h_ )
{
	int x,y;
	int val;
	int max=0;
	for(y=y_;y<y_+h_;y++)
		for(x=x_;x<x_+w_;x++) {
			val = getpixel_ushort( p , w,h, x,y );
			if( val > max )
				max = val;
		}
	return max;
}

int get_img_min( unsigned short *p , int w, int h , int x_ , int y_, int w_ , int h_ )
{
	int x,y;
	int val;
	int min=65536;
	for(y=y_;y<y_+h_;y++)
		for(x=x_;x<x_+w_;x++) {
			val = getpixel_ushort( p , w,h, x,y );
			if( val < min )
				min = val;
		}
	return min;
}

long long best_image_match_( unsigned short *needle, unsigned short *haystack , int w1,int h1, int w2, int h2 ,
		int x_needle, int y_needle , int cmp_w , int cmp_h,  int *xmatch, int *ymatch ,
		int x2_min,int y2_min , int x2_max, int y2_max , int avg_needle )
{	
long long min = -1 ;
int x2,y2;
int xm,ym;
int avg_haystack;
int max_needle = get_img_max( needle, w1,h1 , x_needle, y_needle , cmp_w , cmp_h );
int min_needle = get_img_min( needle, w1,h1 , x_needle, y_needle , cmp_w , cmp_h );
printf("min_needle = %d max_needle = %d\n",min_needle,max_needle );
for(y2=y2_min;y2<y2_max-cmp_h;y2++) {
	for(x2=x2_min;x2<x2_max-cmp_w;x2++) {
		long long diff;
		int min_ = min_needle;
		int max_ = max_needle;
		int max_haystack = get_img_max( haystack, w2,h2 , x2, y2 , cmp_w , cmp_h );
		int min_haystack = get_img_min( haystack, w2,h2 , x2, y2 , cmp_w , cmp_h );
		int avg_haystack = get_img_avg( haystack,w2,h2 , x2, y2, cmp_w , cmp_h );
		if( min_haystack < min_ )
			min_ = min_haystack;
		if( max_haystack > max_ )
			max_ = max_haystack;
		// avg_haystack = get_img_avg( haystack,w2,h2 , x2, y2, cmp_w , cmp_h );
		// avg_needle = 65535;
		// avg_haystack = 65535;
		// if( getpixel_ushort(haystack,w2,h2, x2,y2 ) < 1000 )
		// 	continue;
		diff = image_diff_bounds(haystack,needle,w2,h2,w1,h1, x2,y2, x_needle,y_needle ,  cmp_w,cmp_h , avg_haystack , avg_needle ,
					min_haystack, max_haystack , min_needle , max_needle );
		if( diff == 0 )
			printf("diff = 0 x2=%d y2=%d min=%d max=%d\n",x2,y2 , min_haystack, max_haystack );
		if(min == -1 || diff < min ) {
			min = diff;
			*xmatch = xm = x2;
			*ymatch = ym = y2;
		}
	}
}
printf("done\n");
return min;
}

long long best_image_match_subpixel( unsigned short *needle, unsigned short *haystack , int w1,int h1, 
			int x_needle, int y_needle , int x2 , int y2 , int cmp_w , int cmp_h,  int *xmatch_sub, int *ymatch_sub , int avg_haystack  )
{	
	long long min = -1 ;
	int x2_sub,y2_sub;
	int xm,ym;
	int avg_needle = get_img_avg( needle,w1,h1 , x_needle, y_needle, cmp_w , cmp_h );
	
	for(y2_sub=0;y2_sub<16;y2_sub++) {
		for(x2_sub=0;x2_sub<16;x2_sub++) {
			long long diff;
			// avg_needle = 65536;
			// avg_haystack = 65536;
			diff = image_diff_subpixel(needle,haystack,w1,h1, x_needle,y_needle , x2,y2, cmp_w,cmp_h , x2_sub , y2_sub , avg_needle , avg_haystack );
			if(min == -1 || diff < min ) {
				min = diff;
				*xmatch_sub = xm = x2_sub;
				*ymatch_sub = ym = y2_sub;
			}
		}
	}
	return min;
}


long long image_search2( unsigned short *p, unsigned short *q , int w_p,int h_p, int w_q, int h_q,  float *x , float *y , float *scale , int x0 , int y0 , int x_start,int y_start, int range , float step   )
{
	unsigned short *needle[16][16][16];;
	cmp_size &= 0xfffe;
	double avg,sigma;
	int x_,y_;
	int i,j,k;
	float xmin;
	float ymin;
	int k_min;
	long long min = -1;
	int istart,istep;
	int kstart = 0;
	int kstep = 8;
	unsigned short *q_sub;
	if( step == 1 ) {
		istart = 0;
		istep = 8;
	} else {
		istart = -8;
		istep = 1;
	}
	printf("w_p = %d h_p = %d\n",w_p,h_p );
	printf("w_q = %d h_q = %d\n",w_q,h_q );
	for(k=-8;k<8;k++)
		for(i=-8;i<8;i++)
			for(j=-8;j<8;j++) {
				// needle[i+8][j+8] = downsample( needle16 , w_needle, h_needle , 8-j , 8-i , cmp_size , cmp_size , 16 );
				needle[k+8][i+8][j+8] = get_sub_image( p,w_p,h_p,(x0-j/16.0)*w_p/w_q,(y0-i/16.0)*w_p/w_q,cmp_size/2, (w_p/w_q) * (1+k/16.0/cmp_size)  );
			}
	// normalize_img( needle[8][8][8] , cmp_size , cmp_size , &avg , &sigma ,  0 , 0 );
	for( y_ = y_start-range ; y_ < y_start + range ; y_ ++ )
		for( x_ = x_start-range ; x_ < x_start + range ; x_ ++ ) {
			q_sub = get_sub_image(q,w_q,h_q,x_,y_,cmp_size/2,1);
			// normalize_img( q_sub , cmp_size , cmp_size , NULL , NULL ,  avg , sigma );
			for(k=kstart;k<8;k+=kstep)
			   for(i=istart;i<8;i+=istep)
				for(j=istart;j<8;j+=istep) {
					long long diff = simple_image_diff( q_sub, needle[k+8][i+8][j+8] , cmp_size , cmp_size );
					if( min == -1 || diff < min ) {
						min = diff;
						xmin = x_ + j/16.0;
						ymin = y_ + i/16.0;
						k_min = k;
					}
					unsigned short *q2_sub = get_sub_image(q,w_q,h_q,x_+j/16.0,y_+i/16.0,cmp_size/2,1);
					diff = simple_image_diff( q2_sub, needle[8][8][8] , cmp_size , cmp_size );
					free(q2_sub);
					if(  diff < min ) {
						min = diff;
						xmin = x_ + j/16.0;
						ymin = y_ + i/16.0;
						k_min = 0;
					}
				}
			free(q_sub);
		}
	for(k=0;k<16;k++)
	    for(i=0;i<16;i++)
		for(j=0;j<16;j++)
			free( needle[k][i][j] );
	*x = xmin;	
	*y = ymin;
	*scale = 1/(1+k_min/16.0/(float)cmp_size);
	printf("k_min = %d\n",k_min );
	return min;
}

long long image_search( unsigned short *p, unsigned short *q , int w, int h,  float *x , float *y , int x0 , int y0 , int range   )
{
	int i;
	FILE *f;
	int x2_min,y2_min;
	int x2_max,y2_max;
	x2_min = *x-range;
	x2_max = *x+range;
	y2_min = *y-range;
	y2_max = *y+range;

	if( x2_min < 0 )
		x2_min = 0;
	if( y2_min < 0 )
		y2_min = 0;
	if( x2_max > w )
		x2_max = w;
	if( y2_max > h )
		y2_max = h;
	 {
		long long diff;
		long long diff2;
		int xmatch,ymatch;
		int xmatch_,ymatch_;
		int xmatch_sub,ymatch_sub;
		int xsub;
		int ysub;
		int xm;
		int ym;
		int xneedle;
		int yneedle;
		int avg_needle =  get_img_avg( p,w,h , x0-cmp_size/2, y0-cmp_size/2, cmp_size , cmp_size );
		diff = best_image_match_( p , q , w , h , w, h , x0-cmp_size/2,y0-cmp_size/2,cmp_size,cmp_size,  &xmatch, &ymatch,
					x2_min,y2_min,x2_max,y2_max , avg_needle   );
		printf("diff = %lld %d %d\n",diff,xmatch,ymatch);
		xneedle = x0-cmp_size/2;
		yneedle = y0-cmp_size/2;
		xmatch_sub = 0;
		ymatch_sub = 0;
		diff = -1;
		for(ym=yneedle-1;ym<=yneedle+1;ym++)
			for(xm=xneedle-1;xm<=xneedle+1;xm++) {
				diff2 = best_image_match_subpixel( q , p , w , h ,  xmatch,ymatch, xm,ym, cmp_size,cmp_size,  &xsub, &ysub , avg_needle  );
				if( diff < 0 || diff2 < diff ) {
					diff = diff2;
					xmatch_ = xm;
					ymatch_ = ym;
					xmatch_sub = xsub;
					ymatch_sub = ysub;
				}
			}

		printf("diff = %lld %d %d %d %d\n",diff,xmatch_,ymatch_,xmatch_sub,ymatch_sub);
		xmatch += cmp_size/2;
		ymatch += cmp_size/2;
		xmatch_ += cmp_size/2;
		ymatch_ += cmp_size/2;
		*x =  x0 + xmatch - ( xmatch_+xmatch_sub/16.0 ) ;
		*y =  y0 + ymatch - ( ymatch_+ymatch_sub/16.0 ) ;
		// x_cursor = xmatch+16;
		return diff;
	}
}

float get_distance( float x1,float y1, float x2,float y2 ) 
{
	x1-=x2;
	y1-=y2;
	return ( sqrt( x1*x1+y1*y1 ) );
}

int print_star_distribution( unsigned short *img , int w , int h , int j , float *fwhm , const char *fname   )
{
	int count;
	int x,y;
	int i;
	float cx[4];
	float cy[4];
	float sum1;
	float sum2;
	int r1=cmp_size/2;
	float avg;
		x = cx[j] =  x_cursor[j];
		y = cy[j] =  y_cursor[j];
	return get_psf( img , w ,h , cx[j],cy[j] , r1 , fwhm , fname );
}

void print_3x3( unsigned short *img , int w , int h , int x , int y  )
{
	int i,j;
	for( i= 0; i< 3 ; i++ ) {
		for( j= 0; j< 3 ; j++ ) {
			printf("%6d ", img[(y+i)*w+x+j] );
		}
		printf("\n " );
	}

}

void write_pgm_ushort( unsigned short *out_img , int w , int h , char *fname , int x_ , int y_ , int w_ , int h_   )
{
	FILE *f;
	int x,y;
	f = fopen(fname,"w");
	fprintf(f,"P5\n%d %d\n%d\n",w_,h_,max_pixelvalue);
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


int center_cursors( unsigned short *img , int w, int h , int star  )
{
	int x,y;
	int i,j;
	float cx[4];
	float cy[4];
	float sum1;
	float sum2;
	int r1=cmp_size;
	int col_div = 20;
	int square[4] = {0,0,0,0};
	float max0[4];
	int x0[4];
	int y0[4];
	int r_;
	for( j = 0 ; j<cursor_count ; j++ ) {
		int r1=10;
		int r2=20;
		float fwhm;
		x = x0[j] = x_cursor[j];
		y = y0[j] = y_cursor[j];
		float black_level  = get_otsu_threshold( img ,w,h , x , y , cmp_size/2  );
		printf("black_level = %f\n",black_level );
		sum1 = get_center_of_light( img , w , h , x , y  , cmp_size/2   , &cx[j], &cy[j] ,  black_level , NULL );
		printf("center of light cx %f cy %f\n",cx[j],cy[j]);
		if( star ) {
			cx[j] = floor( cx[j]+0.5 );
			cy[j] = floor( cy[j]+0.5 );
			print_3x3( img, w, h , cx[j]-1,cy[j]-1 );
			calcSubPixelCenter( img , w, h , &cx[j] , &cy[j] , 10 , cx[j]-1,cy[j]-1 );
			printf("subpixel center  cx %f cy %f\n",cx[j],cy[j]);
		}
		x_cursor[j] = cx[j] ;
		y_cursor[j] = cy[j] ;
	}
	return 0;
}

int register_images( unsigned short *img , int w, int h , char *fname , int argc , char **argv )
{
	int x,y;
	int i,j;
	float cx[4];
	float cy[4];
	float sum1;
	float sum2;
	int r1=cmp_size;
	FILE *f;
	FILE *f2;
	FILE *f3;
	FILE *f4;
	unsigned short *p;
	int w_p,h_p;
	int col_div = 20;
	int first_img_idx = 2;
	int square[4] = {0,0,0,0};
	float max0[4];
	double img_avg;
	double img_sigma;
	if ( ref_image_name )  {
		p = read_pgm(  &w_p,&h_p , ref_image_name , &x , &y , black_level );
		// normalize_img( p , w_p , h_p , &img_avg , &img_sigma ,  0 , 0 );
		first_img_idx = 1;
	} else {
		p = img ;	
	}
		
	f = fopen(fname,"w" );
	int x0[4];
	int y0[4];
	fprintf(f,"# cmp_size %d\n",cmp_size );
	fprintf(f,"# black_level %d\n",black_level );
	if( ref_image_name )
		fprintf(f,"#ref: %s ",ref_image_name);
	else
		fprintf(f,"%s ",argv[1]  );
	f2 = fopen("xplot.txt","w");
	f3 = fopen("xplot2.txt","w");
	f4 = fopen("xplot_max.txt","w");
	for( j = 0 ; j<cursor_count ; j++ ) {
		int r1=cmp_size/4;
		int r2=cmp_size/2;
		int count1;
		int count2;
		x = x0[j] = x_cursor[j];
		y = y0[j] = y_cursor[j];
		float blacklevel = print_star_distribution( img , w , h , j , NULL , NULL  );
		printf("black_level = %f\n",blacklevel );
		max0[j] = get_max_point( img , w , h , x , y  , r1   , &cx[j], &cy[j]  );
		sum1 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r1   , &cx[j], &cy[j] ,  0 , &count1 );
		sum2 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r2   , &cx[j], &cy[j] ,  0 , &count2  );
		printf("sum1/count1 = %d (sum2-sum1)/(count2-count1) = %d\n", sum1/count1 , (sum2-sum1)/(count2-count1) );
		if( (sum1/count1) < 4*(sum2-sum1)/(count2-count1)  ) {
			printf("square match\n");
			square[j] = 1;
			cx[j] = x0[j] = x_cursor[j];
			cy[j] = y0[j] = y_cursor[j];
		} else {
			printf("using centroid\n");
			max0[j] = get_max_point( img , w , h , x , y  , r1   , &cx[j], &cy[j]  );
			sum1 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r1   , &cx[j], &cy[j] ,  blacklevel , &count1 );
			sum2 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r2   , &cx[j], &cy[j] ,  blacklevel , &count2  );
			cx[j] = floor( cx[j]+0.5 );
			cy[j] = floor( cy[j]+0.5 );
			print_3x3( img, w, h , cx[j]-1,cy[j]-1 );
			calcSubPixelCenter( img , w, h , &cx[j] , &cy[j] , 10 , cx[j]-1,cy[j]-1 );
		}
		fprintf(f,"%f %f 0  ",cx[j],cy[j] );
	}
	if(!cursor_count)
		fprintf(f,"0 0 0  " );
	fprintf(f2,"%f %f\n",cx[0],cy[0] );
	fprintf(f3,"%f %f\n",cx[0]-floor(cx[0]),cy[0]-floor(cy[0]) );
	if( cursor_count >= 2 )
		printf("distance %f\n",get_distance(cx[0],cy[0],cx[1],cy[1] ) );
	fprintf(f,"\n" );
	printf("get needle pgm\n");
	printf("floor %f %f\n",floor(x_cursor[0]) , floor(y_cursor[0]) );
	{ unsigned short *p_sub = get_sub_image(p,w_p,h_p,floor(x_cursor[0])*w_p/w,floor(y_cursor[0])*w_p/w,(cmp_size/2)*cross_power_factor*w_p/w,1);
	  char out_fname[1024];
	  strcpy(out_fname,"needle.pgm");
	  write_pgm_ushort( p_sub , cmp_size*cross_power_factor*w_p/w , cmp_size*cross_power_factor*w_p/w , out_fname , 0,0,cmp_size*cross_power_factor*w_p/w,cmp_size*cross_power_factor*w_p/w );
	  free(p_sub);
        }
	printf("got needle pgm\n");
	for(i=first_img_idx;i<argc;i++) {
		unsigned short *q;
		float max;
		int j;
		printf("black_level = %d\n",black_level );
		q = read_pgm(  &w,&h , argv[i] , &x , &y , black_level );
		// normalize_img( q , w , h , NULL , NULL ,  img_avg , img_sigma );
		printf("register %s\n",argv[i] );
		fprintf(f,"%s ",argv[i]  );
		for( j = 0 ; j<cursor_count ; j++ ) {
			float scale = 1;
			long long diff = 0;
			float dx = 0,dy = 0;
			if(square[j]) {
				int r = r1;
				float max_x,max_y;
				max = get_max_point( q , w , h , cx[j] , cy[j]  , r1   , &max_x , &max_y  );
				fprintf(f4,"%d %f\n",i,max);
				/*
				if( max <= max0[j]/4 ) {
					r = w+h; 
					printf("full search\n");
				}
				*/
				diff = image_search2( p, q , w_p,h_p, w, h, &cx[j]  , &cy[j] , &scale , x0[j] , y0[j] , x_cursor[0],y_cursor[0] , r , 1  );
				printf("step 1    cx = %f cy = %f diff = %lld\n",cx[j],cy[j] , diff );
#ifndef CROSS_SPECTRUM
				diff = image_search2( p, q , w_p,h_p, w, h, &cx[j]  , &cy[j] , &scale , x0[j] , y0[j] , cx[j],cy[j] , 4 , 1/16.0  );
				printf("step 1/16 cx = %f cy = %f diff = %lld\n",cx[j],cy[j] , diff );
#endif
				//diff = image_search( img, q , w, h, &cx[j]  , &cy[j] , x0[j] , y0[j] , r   );
				diff /= (4*r*r);
				printf("floor %f %f\n",floor(cx[0]) , floor(cy[0]) );
				{ unsigned short *q_sub = get_sub_image(q,w,h,floor(cx[0]),floor(cy[0]),(cmp_size/2)*cross_power_factor,1);
				  FILE *f;
				  char out_fname[1024];
				  sprintf(out_fname,"sub_image.pgm",i);
	  			  write_pgm_ushort( q_sub , cmp_size*cross_power_factor , cmp_size*cross_power_factor , out_fname , 0,0,cmp_size*cross_power_factor,cmp_size*cross_power_factor );
				  free(q_sub);
#ifdef CROSS_SPECTRUM
				  system("register_images.py needle.pgm sub_image.pgm > offsets.txt");
				  f = fopen("offsets.txt","r");
				  if( f ) {
					char dummy[32];
					fscanf(f,"%s %f %s %f %s %lld",dummy,&dx,dummy,&dy , dummy , &diff );
					fclose(f);
					printf("dx = %f dy = %f\n",dx,dy);
					cx[0] = floor(cx[0]) + dx;
					cy[0] = floor(cy[0]) + dy;
					
				  }
#endif
					
				}
			} else {
				int r_;
				printf("center of light\n");
				float black_level = print_star_distribution( img , w , h , j , NULL , NULL   );
				printf("black_level = %f\n",black_level );
				r_ = cmp_size/2;
				max = get_max_point( q , w , h , cx[j] , cy[j]  , r_   , &cx[j], &cy[j] );
				fprintf(f4,"%d %f\n",i,max);
				if( max >= max0[j]/8 ) {
					get_center_of_light( q , w , h , cx[j] , cy[j]  , r_   , &cx[j], &cy[j] , black_level , NULL );
					printf("cx %f cy %f\n",cx[j],cy[j] );
					cx[j] = floor( cx[j]+0.5 );
					cy[j] = floor( cy[j]+0.5 );
					print_3x3( q, w, h , cx[j]-1,cy[j]-1 );
					calcSubPixelCenter( q , w, h , &cx[j] , &cy[j] , 10 , cx[j]-1,cy[j]-1 );
					printf("cx %f cy %f new\n",cx[j],cy[j] );
				} else {
					printf("no star found\n");
					cx[j] = -1;
				}
			}
			x_cursor[j] = cx[j];
			y_cursor[j] = cy[j];
			printf("x_cursor %d %f %f\n",j,x_cursor[j],y_cursor[j] );
			fprintf(f,"%6.2f %6.2f %10lld  ",cx[j],cy[j],diff  );
// #ifdef CROSS_SPECTRUM
//			fprintf(f,"%6.2f %6.2f %10lld  ",floor(cx[j])+dx,floor(cy[j])+dy,diff  );
// #endif
			/*
			if( square[j] ) {
				fprintf(f,"%6.2f %6.2f %10lld  ",cx[j]+scale*cmp_size/2,cy[j],diff  );
				fprintf(f,"%6.2f %6.2f %10lld  ",cx[j],cy[j]+scale*cmp_size/2,diff  );

			}
			*/
			printf("%f %f %lld  ",cx[j]-x0[j],cy[j]-y0[j],diff );
		}
		if(!cursor_count)
			fprintf(f,"0 0 0  " );
		fprintf(f,"\n" );
		printf("\n" );
		if( cursor_count >= 2 )
			printf("distance %f %f %f\n",get_distance(cx[0],cy[0],cx[1],cy[1]) , cx[1]-cx[0],cy[1]-cy[0]  );
		fprintf(f2,"%f %f\n",cx[0],cy[0] );
		fprintf(f3,"%f %f\n",cx[0]-floor(cx[0]),cy[0]-floor(cy[0]) );
		if( cursor_count ) {
			int j;
			SDL_Event event;
			memset( &event, 0 , sizeof(event) );
			printf("showpic\n");
			SDL_SetWindowTitle( window , argv[i] );
			for(j=0;j<1;j++) {
				/*
				showpic_zoom(p,w,h,x0[0]*4-64,y0[0]*4-64,4);
				SDL_WaitEventTimeout(&event,100); 
				if( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q ) {
					break;
				}
				*/
				showpic_zoom(q,w,h,cx[0]*4-64,cy[0]*4-64,4);
				SDL_WaitEventTimeout(&event,100); 
				if( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q ) {
					break;
				}
			}
			if( j<1 )
				break;
		} else {
			showpic(q,w,h);
		}
		// usleep(200000);
		free(q);
	}
	if( p != img )
		free( p );
	fclose(f);
	fclose(f2);
	fclose(f3);
	fclose(f4);
	sleep(1);
	cursor_count = 0;
	showpic(img,w,h);

}

void read_register_file( const char *name )
{
	FILE *f;
	char line[1024];
	char s[32];
	f = fopen(name,"r");
	if(!f) 
		exit(1);
	fgets(line,256,f);
	sscanf(line+1,"%s %d",s,&cmp_size ); 
	fgets(line,256,f);
	sscanf(line+1,"%s %d",s,&black_level ); 
	fgets(line,1024,f);
	sscanf(line+5,"%s %f %f",ref_image_name_,&x_cursor[0],&y_cursor[0] ); 
	ref_image_name = ref_image_name_;
	printf("cmp_size = %d\n",cmp_size );
	printf("black_level = %d\n",black_level );
	printf("ref %s %f %f\n",ref_image_name , x_cursor[0],y_cursor[0] );
	cursor_count = 1;
}

int main( int argc,char **argv )
{
	int done = 0;
	int w,h,x,y;
	float cx,cy;
	SDL_Event event;
	int i;
	char *p;
	char channel[3] = {0};
	FILE *f;
	char fname[1024];
	while( argv[1][0] == '-' ) {
		if( strcmp(argv[1],"-h") == 0 ) {
			printf("usage:select_star -blacklevel x -ref ref_pgm_name img1 img2 ...\n");
			printf("usage:select_star -register register.txt  img1 img2 ...\n");
			exit(1);
		}
		if( strcmp(argv[1],"-blacklevel")==0 ) {
			black_level = atoi(argv[2]) ;
			argc -= 2;
			argv += 2;
		}
		if( strcmp(argv[1],"-ref")==0 ) {
			ref_image_name = argv[2];
			argc -= 2;
			argv += 2;
		}
		if( strcmp(argv[1],"-register")==0 ) {
			register_in_name = argv[2];
			argc -= 2;
			argv += 2;
			read_register_file(register_in_name);
		}
	}
	printf("reading img\n");
	unsigned short *img = read_pgm(  &w,&h , argv[1] , &x_offset , &y_offset , black_level );
	printf("w=%d h=%d img=%p\n",w,h,img);
	if( !no_ui )
		init_screen(w,h);
	printf("showpic\n");
	showpic( img , w, h );
	printf("showpic done\n");
	strcpy(fname,argv[1] );
	p = rindex(fname,'_');
	
	if( p && ( p[1] == 'r' || p[1] == 'g' || p[1] == 'b' )  ) {
		channel[0] = p[1];
		channel[1] = p[2];
		channel[2] = 0;
	}
	printf("2\n");
	if( !p )
		p = rindex(fname,'.');
	printf("3\n");
	if( !p )
		exit(1);
	sprintf(p,"_register_%s.txt",channel);
	printf("4\n");
	if( register_in_name ) {
		register_images( img , w , h , fname , argc, argv  );
		exit(0);
	}
	while ( !done && SDL_WaitEvent(&event) ) {
		switch (event.type) {
			case SDL_MOUSEBUTTONDOWN:
				printf("mouse %d %d\n", event.button.x, event.button.y );
				if( cursor_count == 3 )
					cursor_count = 0;
				x_cursor[cursor_count] = event.button.x;
				y_cursor[cursor_count++] = event.button.y;
				showpic(img,w,h);
				break;
			
				break;
			case SDL_KEYUP:
				if ( event.key.keysym.sym == SDLK_q ) {
					printf("space\n");
					done = 1;
					break;
				}
				if ( event.key.keysym.sym == SDLK_m ) {
					method[cursor_count] = !method[cursor_count];
					printf("method = %d\n",method[cursor_count] );
					break;
				}
				if ( event.key.keysym.sym == SDLK_KP_MULTIPLY ) {
					cmp_size *= 2;
					showpic( img , w, h );
					break;
				}
				if ( event.key.keysym.sym == SDLK_KP_DIVIDE ) {
					cmp_size /= 2;
					showpic( img , w, h );
					break;
				}
				if ( event.key.keysym.sym == SDLK_KP_PLUS ) {
					cmp_size+=4;
					showpic( img , w, h );
					break;
				}
				if ( event.key.keysym.sym == SDLK_KP_MINUS ) {
					cmp_size -= 4;
					showpic( img , w, h );
					break;
				}
				if ( event.key.keysym.sym == SDLK_PLUS ) {
					div_/=2 ;
					if(div_==0)
						div_=1;
					printf("div=%d\n",div_);
						showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_MINUS ) {
					div_*=2 ;
					printf("div=%d\n",div_);
						showpic(img,w,h);

					break;
				}
				if ( event.key.keysym.sym == SDLK_r ) {
					register_images( img , w , h , fname , argc, argv  );
					break;
				}
				if ( event.key.keysym.sym == SDLK_c ) {
					center_cursors( img , w , h , 1  );
					showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_x ) {
					center_cursors( img , w , h , 0  );
					showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_p ) {
					char fname[1024];
					char *p;
					int blacklevel;
					strcpy(fname,argv[1] );
					p = rindex(fname,'.');
					if( p ) {
						strcpy(p,"_psf.txt");
					}
					blacklevel = print_star_distribution( img , w , h , 0 , NULL , p ? fname : NULL  );
					if( p ) {
						strcpy(p,"_psf.pgm");
					}
					get_psf3( img , w ,h ,  x_cursor[0]  , y_cursor[0]  , cmp_size/2 , blacklevel , fname );
					cursor_count = 0;
					showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_o ) {
					int i;
					int o = get_otsu_threshold( img ,w,h , x_cursor[0] , y_cursor[0] , cmp_size/2  );
					printf("otsu threshold = %d\n", o );
					black_level += o;
					free(img);
					img = read_pgm(  &w,&h , argv[1] , &x_offset , &y_offset , black_level );
					cursor_count = 0;
					showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_b ) {
					int i;
					// int o = get_otsu_threshold( img ,w,h , x_cursor[0] , y_cursor[0] , cmp_size/2  );
					int o = get_img_avg( img,w,h , x_cursor[0]-cmp_size/2, y_cursor[0]-cmp_size/2, cmp_size , cmp_size );
					printf("avg threshold = %d\n", o );
					black_level += o;
					free(img);
					img = read_pgm(  &w,&h , argv[1] , &x_offset , &y_offset , black_level );
					cursor_count = 0;
					showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_g ) {
					remove_gradient(img,w,h, x_cursor[0],y_cursor[0] );
					cursor_count = 0;
					showpic(img,w,h);
					break;
				}
				if ( event.key.keysym.sym == SDLK_s ) {
				  float zoom = 1;
				  unsigned short *p_sub = get_sub_image(img,w,h,floor(x_cursor[0]),floor(y_cursor[0]),cmp_size*zoom,1/zoom);
				  write_pgm_ushort( p_sub , cmp_size*2*zoom , cmp_size*2*zoom , "sub1.pgm" , 0,0,cmp_size*2*zoom,cmp_size*2*zoom );
				  free(p_sub);
				  img = read_pgm(  &w,&h , argv[2] , &x_offset , &y_offset , black_level );
				  p_sub = get_sub_image(img,w,h,floor(x_cursor[0]),floor(y_cursor[0]),cmp_size*zoom,1/zoom);
				  write_pgm_ushort( p_sub , cmp_size*2*zoom , cmp_size*2*zoom , "sub2.pgm" , 0,0,cmp_size*2*zoom,cmp_size*2*zoom );
				  free(p_sub);
				  img = read_pgm(  &w,&h , argv[1] , &x_offset , &y_offset , black_level );
				}
				break;
		}
	}

}


