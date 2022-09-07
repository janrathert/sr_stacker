
/* Simple program:  Test bitmap blits */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_syswm.h"

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
	SDL_Quit();
	exit(rc);
}

static SDL_Surface *screen;
static Uint8 *buffer;

int init_screen()
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
	x = 1920-64*5;
	y = 1080/2-64*2;
	SDL_putenv("SDL_VIDEO_WINDOW_POS=1600,500");

	if ( (screen=SDL_SetVideoMode(64*4,64*4,video_bpp,videoflags)) == NULL ) {
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



void showpic( unsigned short img[3056][4056] , int x , int y , int zoom  )
{
	int i,j;
	printf("x = %d y = %d zoom = %d\n",x,y,zoom);
	if(x<0)
		x=0;
	if(y<0)
		y=0;
	if(x+screen->w/zoom >= 4056/2)
		x= 4056/2 - screen->w/zoom ; 
	if(y+screen->h/zoom >= 3056/2)
		y= 3056/2 - screen->h/zoom ; 


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
				buffer[4*j]=img[(y+i/zoom)*2][(x+j/zoom)*2]/256;
				buffer[4*j+1]=img[(y+i/zoom)*2][(x+j/zoom)*2]/256;
				buffer[4*j+2]=img[(y+i/zoom)*2][(x+j/zoom)*2]/256;
				buffer[4*j+3]=255;
			}
        		// memset(buffer,(i*255)/screen->h, screen->pitch);
        		buffer += screen->pitch;
        	}
	SDL_UnlockSurface(screen);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

}
