#include <stdio.h>

unsigned short* read_pgm(  int *w , int *h,  const char *fname )
{
	int i;
	FILE *f;
	int max;
	int w_,h_;
	int m=0;
	char line[256];
	unsigned short *r;
	if(strcmp(fname,"stdin")== 0 )
		f = stdin;
	else
		f = fopen(fname,"r");
	if(!f)
		return NULL;
	fgets(line,sizeof(line),f);
	do {
		fgets(line,sizeof(line),f);
	} while(line[0] == '#' );
	sscanf(line,"%d %d",&w_,&h_);
	fgets(line,sizeof(line),f);
	sscanf(line,"%d",&max);
	fprintf(stderr,"max = %d w=%d h=%d\n",max,w_,h_); r = malloc( w_ * h_ * sizeof(short) );
	for(i=0;i<w_*h_ && !feof(f);i++) {
		unsigned char p1 = 0;
		if( max > 255 )
			p1 = fgetc(f);
		unsigned char p2 = fgetc(f);
		r[i] = p1*256 + p2 ;
		if( r[i] > m )
			m = r[i];
	}
	fprintf(stderr,"m=%d\n",m);
	*w = w_;
	*h = h_;
	if( f != stdin )
		fclose(f);
	return r;
}

int main( int argc,char **argv )
{
	int w,h;
	FILE *f;
	int x,y;
	char fname[1024];
	char out_fname[1024];
	int i ;
	int j;
	char c1[4] = {'g','b','r','g' };
	char c2[4] = {'1','1','1','2' };
	for(j=1;j<argc;j++) {
		fprintf(stderr,"open %s\n",argv[j] );
		unsigned short *p = read_pgm(&w,&h,argv[j] );
		if(!p) {
			fprintf(stderr,"error open file\n");
			continue;
		}
		
		for(i=0;i<4;i++) {
			sprintf(out_fname,"out_%c%c_%06d.pgm",c1[i],c2[i],j);
			f = fopen(out_fname,"w");
			fprintf(f,"P5\n%d %d\n65535\n",w/2,h/2);
			for(y=0;y<h/2;y++) {
				for(x=0;x<w/2;x++) {
					unsigned short bayer[4];
					int value = 0;
					bayer[0] = p[(2*y)*w+2*x];
					bayer[1] = p[(2*y)*w+2*x+1];
					bayer[2] = p[(2*y+1)*w+2*x];
					bayer[3] = p[(2*y+1)*w+2*x+1];
					value = bayer[i] ;
					if( value > 65535 )
						value = 65535;
					fputc( value/256 , f );
					fputc( value & 255 , f );
					/*
					fputc( bayer[2] , f );
					fputc( bayer[3] , f );
					fputc( bayer[1] , f );
					*/
				}
			}
			fclose(f);
		}
		free(p);
	}
}

