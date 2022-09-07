
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "image_match.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include "track_star.h"
int black_level = 0;

int max_pixelvalue = 255;
int max_x = 0;
int max_y = 0;
int div_=256;
int cmp_size = 32;
int x_offset;
int y_offset;

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


/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
	SDL_Quit();
	exit(rc);
}

static SDL_Surface *screen;
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

	if ( (screen=SDL_SetVideoMode(w,h,video_bpp,videoflags)) == NULL ) {
		fprintf(stderr, "Couldn't set 640x480x%d video mode: %s\n",
						video_bpp, SDL_GetError());
		quit(2);
	}
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
static int x_cursor[4] = {-1,-1,-1,-1} ;
static int y_cursor[4] ;
static int method[4]={1,1,1,1};
static int cursor_count = 0;



static inline void set_pixel(int x,int y, int c)
{
	unsigned char *buf = (Uint8 *) screen->pixels;
	buf += screen->pitch *y + 4*x;
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


void showpic( unsigned short *img , int w , int h   )
{
	int i,j;

	/* Set the surface pixels and refresh! */
	if ( SDL_LockSurface(screen) < 0 ) {
		fprintf(stderr, "Couldn't lock the display surface: %s\n",
							SDL_GetError());
		quit(2);
	}
	buffer=(Uint8 *)screen->pixels;
		
        	for ( i=0; i<screen->h; ++i ) {
			int j;
			for(j=0;j<screen->w;j++) {
				int val = img[i*w+j]/div_;
				if( val > 255 )
					val = 255;
				buffer[4*j]=val;
				buffer[4*j+1]=val;
				buffer[4*j+2]=val;
				buffer[4*j+3]=255;
			}
        		// memset(buffer,(i*255)/screen->h, screen->pitch);
        		buffer += screen->pitch;
        	}
	draw_cursors();
	SDL_UnlockSurface(screen);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

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
int center_cursors( unsigned short *img , int w, int h  )
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
	for( j = 0 ; j<cursor_count ; j++ ) {
		int r1=10;
		int r2=20;
		x = x0[j] = x_cursor[j];
		y = y0[j] = y_cursor[j];
		max0[j] = get_max_point( img , w , h , x , y  , r1   , &cx[j], &cy[j]  );
		sum1 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r1   , &cx[j], &cy[j] ,  max0[j]/col_div );
		x_cursor[j] = cx[j];
		y_cursor[j] = cy[j];
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
	int col_div = 20;
	int square[4] = {0,0,0,0};
	float max0[4];
	f = fopen(fname,"w" );
	int x0[4];
	int y0[4];
	fprintf(f,"%s ",argv[2]  );
	f2 = fopen("xplot.txt","w");
	f3 = fopen("xplot2.txt","w");
	for( j = 0 ; j<cursor_count ; j++ ) {
		int r1=10;
		int r2=20;
		x = x0[j] = x_cursor[j];
		y = y0[j] = y_cursor[j];
		max0[j] = get_max_point( img , w , h , x , y  , r1   , &cx[j], &cy[j]  );
		sum1 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r1   , &cx[j], &cy[j] ,  max0[j]/col_div );
		sum2 = get_center_of_light( img , w , h , cx[j] , cy[j]  , r2   , &cx[j], &cy[j] ,  max0[j]/col_div );
		if( method[j] ) {
			printf("square match\n");
			square[j] = 1;
			cx[j] = x0[j] = x_cursor[j];
			cy[j] = y0[j] = y_cursor[j];
		}
		fprintf(f,"%f %f 0  ",cx[j],cy[j] );
	}
	if(!cursor_count)
		fprintf(f,"0 0 0  " );
	fprintf(f2,"%f %f\n",x_offset+cx[0],y_offset+cy[0] );
	fprintf(f3,"%f %f\n",cx[0]-floor(cx[0]),cy[0]-floor(cy[0]) );
	if( cursor_count >= 2 )
		printf("distance %f\n",get_distance(cx[0],cy[0],cx[1],cy[1] ) );
	fprintf(f,"\n" );
	for(i=3;i<argc;i++) {
		unsigned short *q;
		float max;
		int j;
		q = read_pgm(  &w,&h , argv[i] , &x , &y );
		printf("register %s\n",argv[i] );
		fprintf(f,"%s ",argv[i]  );
		for( j = 0 ; j<cursor_count ; j++ ) {
			long long diff = 0;
			if(square[j]) {
				int r = r1;
				float max_x,max_y;
				max = get_max_point( q , w , h , cx[j] , cy[j]  , r1   , &max_x , &max_y  );
				if( max <= max0[j]/4 ) {
					r = w+h; 
					printf("full search\n");
				}
				diff = image_search( img, q , w, h, &cx[j]  , &cy[j] , x0[j] , y0[j] , r  );
			} else {
				printf("center of light\n");
				max = get_max_point( q , w , h , cx[j] , cy[j]  , r1   , &cx[j], &cy[j] );
				if( max >= max0[j]/8 ) {
					get_center_of_light( q , w , h , cx[j] , cy[j]  , r1   , &cx[j], &cy[j] , max/col_div );
				} else {
					printf("no star found\n");
					cx[j] = -1;
				}
			}
			x_cursor[j] = cx[j];
			y_cursor[j] = cy[j];
			printf("x_cursor %d %d %d\n",j,x_cursor[j],y_cursor[j] );
			fprintf(f,"%f %f %lld  ",cx[j],cy[j],diff );
			printf("%f %f %lld  ",cx[j]-x0[j],cy[j]-y0[j],diff );
		}
		if(!cursor_count)
			fprintf(f,"0 0 0  " );
		fprintf(f,"\n" );
		printf("\n" );
		if( cursor_count >= 2 )
			printf("distance %f %f %f\n",get_distance(cx[0],cy[0],cx[1],cy[1]) , cx[1]-cx[0],cy[1]-cy[0]  );
		fprintf(f2,"%f %f\n",x+cx[0],y+cy[0] );
		fprintf(f3,"%f %f\n",cx[0]-floor(cx[0]),cy[0]-floor(cy[0]) );
		printf("showpic\n");
		showpic(q,w,h);
		free(q);
	}
	fclose(f);
	fclose(f2);
	fclose(f3);
	sleep(1);
	cursor_count = 0;
	showpic(img,w,h);

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
	black_level = atoi( argv[1] );
	FILE *f;
	char fname[1024];
	unsigned short *img = read_pgm(  &w,&h , argv[2] , &x_offset , &y_offset );
	printf("w=%d h=%d img=%p\n",w,h,img);
	init_screen(w,h);
	printf("showpic\n");
	showpic( img , w, h );
	printf("showpic done\n");
	strcpy(fname,argv[2] );
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
			case SDL_KEYDOWN:
				if ( event.key.keysym.sym == SDLK_q ) {
					printf("space\n");
					done = 1;
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
					cmp_size+=2;
					showpic( img , w, h );
					break;
				}
				if ( event.key.keysym.sym == SDLK_KP_MINUS ) {
					cmp_size -= 2;
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
					center_cursors( img , w , h  );
					showpic(img,w,h);
					break;
				}
				break;
		}
	}

}


