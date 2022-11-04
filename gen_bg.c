#include <stdio.h>


static unsigned char img[1080][1920];

void write_file( const char *fname   )
{	
	FILE *out;
	int x,y;
	
	out = fopen(fname,"w");
	if(!out)
		exit(1);
		fprintf(out,"P6\n%d %d\n%d\n",1920,1080,255);
		for(y=0;y<1080;y++) {
			for(x=0;x<1920;x++) {
				fputc( img[y][x] , out );
				fputc( img[y][x] , out );
				fputc( img[y][x] , out );
			}

		}
		fclose(out);
}


void draw_frame( int x1,int y1,int x2,int y2 , int l )
{
	int i;
	for(i=0;i<l;i++) {
		img[y1][x1+i] = 255;
		img[y1+i][x1] = 255;
		img[y2][x1+i] = 255;
		img[y2-i][x1] = 255;
		img[y1][x2-i] = 255;
		img[y1+i][x2] = 255;
		img[y2][x2-i] = 255;
		img[y2-i][x2] = 255;
	}

}

int main()
{
	int x1,y1;
	int x2,y2;
	int i;

	x1 = 400;
	y1 = 0;
	x2 = 400 + 1440;
	y2 = 1080;
	draw_frame( x1,y1,x2,y2-1 , 50 );
	for(i=0;i<3;i++) {
		int w = x2-x1;
		int h = y2-y1;
		int xc = (x1+x2)/2;
		int yc = (y1+y2)/2;
		x1 = xc - w/4;
		x2 = xc + w/4;
		y1 = yc - h/4;
		y2 = yc + h/4;
		
		draw_frame( x1,y1,x2,y2 , 20 );
		draw_frame( x1-1,y1-1,x2+1,y2+1 , 20 );
	}

	
	write_file( "bg.ppm"  );

}
