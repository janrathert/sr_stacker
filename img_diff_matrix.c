
#include <stdio.h>
#include <math.h>
#include "circle.h"
#include "image_match.h"

int max_pixelvalue = 255;
int max_x = 0;
int max_y = 0;

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


void write_pgm_stack( int *out_img , int w , int h , char *fname , int count , int dx , int dy  )
{
	FILE *f;
	int x,y;
	if(!count)
		return;
	f = fopen(fname,"w");
	fprintf(f,"P5\n%d %d\n%d\n",w,h,max_pixelvalue);
	for(y=0;y<h;y+=1)
		for(x=0;x<w;x+=1) {
			int value = getpixel_uint( out_img ,w ,h , x+dx,y+dy ) / count  ;
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



int main(int argc,char **argv)
{
	int i;
	int j;
	int w,h;
	int stack_count = 0;
	static float matrix[100][100];
	static int count[100];
	static int x_offset[100];
	static int y_offset[100];
	static int xmatch_[100][100];
	static int ymatch_[100][100];
	float threshold = atoi(argv[1]);
	argc--;
	argv++;
	for(i=1;i<argc;i++) {
		int k;
		unsigned short *p = read_pgm(&w,&h,argv[i] , x_offset+i , y_offset+i );
		unsigned int *s = malloc(w*h*sizeof(int) );
		char fname[256];
		printf("x_offset %d y_offset %d\n",x_offset[i],y_offset[i] );
		memset( s,0,w*h*sizeof(int) );
		stack_count = 0;
		for(j=1;j<argc;j++) {
			long long diff;
			unsigned short *q = read_pgm(&w,&h,argv[j] , NULL , NULL );
			int xmatch,ymatch;
			diff = best_image_match( p , q , w , h ,w , h , 16 , 16 , 16 , 16 , 32 , 32 , &xmatch, &ymatch );
			printf("xmatch %d ymatch %d\n",xmatch,ymatch);
			xmatch_[i][j] = xmatch;
			ymatch_[i][j] = ymatch;
			printf("diff %d %d %s %s = %lld\n",i,j,argv[i],argv[j],diff);
			matrix[i][j] = diff/1000000.0;
			if( matrix[i][j] < threshold ) {
				int x,y;
				for(y=0;y<h;y+=1) {
					for(x=0;x<w;x+=1) {
						s[y*w+x] += getpixel_ushort( q , w,h, x+xmatch-16,y+ymatch-16 );
					}
				}
				
				stack_count++;
			}
		}
		sscanf(argv[i]+5,"%d",&k);
		printf("k=%d %s\n",k,argv[i]);
		sprintf(fname,"stack%04d_%02d.pgm",k,stack_count);
		unlink(fname);
		if( stack_count >= 3 )
			write_pgm_stack(s,w,h,fname,stack_count , 0 , 0 );
		count[i] = stack_count;
		free(s);
	}
	for(i=1;i<argc;i++) {
		int stack_count = 0;
		for(j=1;j<argc;j++) {
			printf("%7.3f ",matrix[i][j]);

		}
		printf("\n");
	}
	while(1) {
		int max=0;
		char fname[256];
		int k;
		unsigned int *s = NULL;
		stack_count = 0;
		for(j=1;j<argc;j++) {
			if( count[j] > max ) {
				max = count[j];
				i = j;
			}
		}
		printf("max = %d %d\n",i,max);
		if(!max)
			break;
	
		for(j=1;j<argc;j++) {
			if(  matrix[i][j]<threshold) {
				int l;
				int x,y;
				int xmatch = xmatch_[i][j];
				int ymatch = ymatch_[i][j];
				int dx,dy;
				sscanf(argv[j]+5,"%d",&k);
				printf("k=%d %s\n",k,argv[i]);
				sprintf(fname,"out_%06d.pgm",k-1);
				printf("%s ",argv[j]);
				count[j] = 0;
				for(l=1;l<argc;l++)
					matrix[l][j] = threshold;
				unsigned short *p = read_pgm(&w,&h,fname , NULL , NULL );
				if(!s) {
 					s = malloc(w*h*sizeof(int) );
					memset(s,0,w*h*sizeof(int) );
				}
				printf("x_offset %d\n",x_offset[j]);
				printf("y_offset %d\n",y_offset[j]);
				dx = xmatch-16 + x_offset[j] - x_offset[1];
				dy = ymatch-16 + y_offset[j] - y_offset[1];
				printf("dx = %d dy = %d\n",dx,dy );
				for(y=0;y<h;y+=1) {
					for(x=0;x<w;x+=1) {
						s[y*w+x] += getpixel_ushort( p , w,h, x+dx, y+dy );
					}
				}
				
				stack_count++;
			}
		}
		printf("\n");
		count[i] = 0;
		if( stack_count >= 3 ) {
			sprintf(fname,"stack_large_%04d_%02d_00.pgm",i,stack_count);
			unlink(fname);
			write_pgm_stack(s,w,h,fname,stack_count , 0,0 );
			sprintf(fname,"stack_large_%04d_%02d_01.pgm",i,stack_count);
			unlink(fname);
			write_pgm_stack(s,w,h,fname,stack_count , 1,0 );
			sprintf(fname,"stack_large_%04d_%02d_10.pgm",i,stack_count);
			unlink(fname);
			write_pgm_stack(s,w,h,fname,stack_count , 0,1 );
			sprintf(fname,"stack_large_%04d_%02d_11.pgm",i,stack_count);
			unlink(fname);
			write_pgm_stack(s,w,h,fname,stack_count , 1,1 );
		}
		free(s);
		s = NULL;

	}
}
