
#include <stdio.h>
#include <math.h>
#include <strings.h>

int max_pixelvalue = 255;
int max_x = 0;
int max_y = 0;
int scale[3] = { 1,1,1};
int scale_ = 0;

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
	r = malloc( w_ * (h_+5) * sizeof(short) );
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
			r[k] = v = ( ( p1*256 + p2 ) - 4130 ) * scale_  ;
			if( v <  0)
				r[k] = 0;
			if( v > max_pixelvalue ) {
				v = max_pixelvalue;
				r[k] = v;
			}
			k++;
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


void write_ppm_stack( unsigned short *out_img , int w , int h , char *fname ,  int dx , int dy , int *max  )
{
	FILE *f;
	int i;
	int c = 0;
	f = fopen(fname,"w");
	fprintf(f,"P6\n%d %d\n%d\n",w,h,max_pixelvalue);
	for(i=0;i<w*h*3;i++) {
		int value = out_img[i]  * 50000.0 / max[c];
		if( value > 65535 )
			value = 65535;
		if( max_pixelvalue > 255 )
			fputc( value/256 , f);
		fputc( value&255 , f);
		c++;
		if(c==3)
			c = 0;
		}
	fclose(f);
}



int main(int argc,char **argv)
{
	char line[1024];
	FILE *f;
	int xo0,yo0;
	unsigned short *s = NULL;
	int w,h;
	int stack_count = 0;
	char *p;
	int i=0;
	char fname[1024];
	unsigned short *img[3];
	int max[3] = {0};
	strcpy(fname,argv[4] );
	int ox[] = {0,0,0};
	int oy[] = {0,0,0};
	if( argc >= 8 ) {
		scale[0] = atoi( argv[5] );
		scale[1] = atoi( argv[6] );
		scale[2] = atoi( argv[7] );
	}
	if( argc >= 12 ) {	
		ox[1] = atoi(argv[8]);
		oy[1] = atoi(argv[9]);
		ox[2] = atoi(argv[10]);
		oy[2] = atoi(argv[11]);
	}
	argc--;
	argv++;
	for(i=0;i<3;i++) {
		scale_ = scale[i];
	
		unsigned short *p = read_pgm(&w,&h,argv[i] , NULL , NULL );
		p += w*i;
		img[i] = p;
		if(!s) {
			s = malloc(w*h*sizeof(short)*3 );
		}
		// xo = xo0;
		// yo = yo0;
		{
				int x,y;
				for(y=0;y<h;y+=1) {
					for(x=0;x<w;x+=1) {
						s[(y*w+x)*3+i] = getpixel_ushort( p , w,h, x+ox[i],y+oy[i] );
						if(  s[(y*w+x)*3+i] > max[i] )
							max[i] = s[(y*w+x)*3+i] ;
					}
				}
				
				stack_count++;
		}
	}
	printf("stack_count = %d w=%d h=%d\n",stack_count ,w ,h  );
	write_ppm_stack(s,w,h,fname , 0 , 0 , max );
}
