
#include <stdio.h>
#include <math.h>
#include <strings.h>

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


void write_pgm_stack( int *out_img , int w , int h , char *fname , int count , int dx , int dy , int dx_ , int dy_  )
{
	FILE *f;
	int x,y;
	if(!count)
		return;
	f = fopen(fname,"w");
	fprintf(f,"P5\n#offset %d %d\n%d %d\n%d\n",dx+dx_,dy+dy_,w,h,max_pixelvalue);
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
	char line[1024];
	FILE *f;
	unsigned int *s = NULL;
	int w,h;
	int i,j;
	int x,y;
	int stack_count = 0;
	char *p;
	char fname[1024];
	int max = 0;
	int xo=0;
	int yo=0;
	int w_,h_;
	int black_level = 4100;
	unsigned short *flat = read_pgm(&w_,&h_,argv[1] , NULL , NULL );
	unsigned short *bias = NULL;
	if(!flat ) {
		printf("read error 1\n");
		exit(1);
	}
	if( strlen(argv[2])<=5 ) {
		int v = black_level = atoi(argv[2]);
		bias = malloc(w_*h_*2);
		for( j = 0;j<w_*h_;j++) 
			bias[j] = v;
	} else bias = read_pgm(&w_,&h_,argv[2] , NULL , NULL );
	s = malloc(w_*h_*sizeof(int) );
	argc--;
	argv++;
	argc--;
	argv++;
	for( j = 0;j<w_*h_;j++) 
		if(flat[j] > max )
			max = flat[j];
	max -= black_level;
	printf("max = %d\n",max );
	for( i = 1; i<argc; i++ ) {
		int j;
		strcpy(fname,argv[i] );
		p = rindex(fname,'.');
		if(!p)
			exit(1);
		strcpy(p,"_flat_done.pgm");
		unsigned short *p = read_pgm(&w,&h,argv[i] , &xo , &yo );
		printf("xo = %d yo = %d\n",xo,yo );
		for( y = 0;y<h;y++) 
			for(x=0;x<w;x++)
				if( flat[(y+yo)*w_+x+xo] - black_level > 0 ) {
					s[y*w+x] = 4000 +  ( ( p[y*w+x] - bias[ (y+yo)*w_+x+xo ]  ) *  max ) / ( flat[(y+yo)*w_+x+xo] - black_level );
				}
		write_pgm_stack(s,w,h,fname,1 , 0,0,  xo , yo );
	}
}
