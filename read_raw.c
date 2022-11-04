#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>
#include "showpic.h"


// hq camera chip size 6.287mm x 4.712mm

unsigned int img[3056][4056] = { 0 };
	int rgb=0;
	int max = 65535;
	int idx=0;
	int scale=1;
	int flip=0;
	int xmin=0;
	int ymin=0;
	int xmax=0;
	int ymax=0;
	int x_maxval;
	int y_maxval;
	int zoom=0;
	int stack=0;
	int stack_count=0;
	int threshold = 15000;

char prefix[128] = "out";

int x_max = 0;
int y_max = 0;
int x_min = 4056;
int y_min = 3056;
int blacklevel = 0;
int raw_w;
int raw_h;
int raw_xo = 0;
int raw_yo = 0;
int autocrop = 0;
int oddy = 0;

void find_max(int *x_,int *y_)
{
	int i,j;
	int max=0;
	int x,y;
	for(i=0;i<3056;i+=2)
		for(j=0;j<4056;j+=2) 
			if( img[i][j] > max ) {
				max = img[i][j];
				y=i;
				x=j;
			}
	*x_ = x;
	*y_ = y;
	printf("max = %d\n",max);
}

void find_data_range()
{
	int i,j;
	for(i=0;i<3056-16;i++)
		for(j=0;j<4056;j++) 
			if( img[i][j] > threshold ) {
				if( j>x_max )
					x_max = j;
				if( j<x_min )
					x_min = j;
				if( i>y_max )
					y_max = i;
				if( i<y_min )
					y_min = i;
			}
}


int read_dir( char *newest_ )
{
    DIR* dir = NULL;
    struct stat st;
    struct dirent* entry = NULL;
    char second[256] = {0}; 
    char newest[256] = {0}; 
    int ret = -1;
    strcpy(newest,newest_ );
    dir = opendir( "."  );
    if( dir )
    {
        while( ( entry = readdir( dir ) ) != NULL )
        {
            // printf("readdir %s\n",entry->d_name);
	
	    if( strcmp( entry->d_name + strlen(entry->d_name)-4,".raw"   ) == 0 )  {
		if( strcmp(  entry->d_name , newest  ) > 0 ) {
			strcpy( second , newest );
			strcpy(newest,entry->d_name);
			ret = 0;
		}

	    }
        }
	
        closedir( dir );
    }
    if( ret < 0 )
	return ret;
    if( stat(newest,&st) == 0
    	 && st.st_size == 18613248 ) {
    } else {
	ret = -1;
        if( stat(second,&st) == 0 && st.st_size == 18613248 ) {
	     ret = 0;
	     strcpy(newest,second);
	}

    }
    if( ret == 0 ) {
	int diff = strcmp( newest , newest_ );
	if( diff <= 0 )
		return -1;
	strcpy( newest_ , newest );
    }
    return ret;
}

int read_raw( const char *fname )
{
	unsigned char header[32768] = {0};
	unsigned char buf[(4056*12)/8];
	int x,y;
	int i=0;
	short w=4056,h=3056;
		struct stat st;
		static FILE *f;
		if( fname ) {
			if( stat( fname , &st ) != 0 )
				return -1;

			f = fopen(fname,"r");
			if(!f)
				exit(1);
			printf("size %d\n",st.st_size);
			if( st.st_size >= 18711040 && strcmp(fname+strlen(fname)-4,".jpg")== 0  )
				fseek(f,-18711040, SEEK_END);
		}
		fread(header,1,32768,f);
		// 00000d0: 07ec 05f0 0000 0000 0000 0000 0000 0000 ( w , h , little endian )
		memcpy(&w,header+0xd0,2);
		memcpy(&h,header+0xd2,2);
		fprintf(stderr,"header:%s w=%d h=%d\n",header , w, h);
		if( strncmp( header , "BRCM" , 4 ) != 0 ) {
			fclose(f);
			return -1;
		}
		flip=0;
	if( oddy ) {
		fread(buf,1,(w*12)/8,f);
		fseek(f,12+16,SEEK_CUR);
		h--;
	}
	if( xmax == 0 ) {
		xmax=w/2;
		ymax=h/2;
	}
		raw_xo = w/4;
		raw_yo = (3040-h)/2;
	raw_w = w;
	raw_h = h;
		for(y=0;y<h;y++) {
			int j;
			fread(buf,1,(w*12)/8,f);
			
			for(j=0,x=0;x<w;j+=3) {
				int p1,p2,p3;
				p1 = buf[j]*256;
				p2 = buf[j+1]*256;
				p3 = buf[j+2];
				p2 += (p3/16)*16;
				p1 += (p3%16)*16;
				p1 -= blacklevel;
				p2 -= blacklevel;
				if(p1<0)
					p1=0;
				if(p2<0)
					p2=0;
				p1 *= scale;
				p2 *= scale;
				if(p1>65535)
					p1=65535;
				if(p2>65535)
					p2=65535;
				if( flip ) {
					img[3055-y][4055-x++] = p1;
					img[3055-y][4055-x++] = p2;
				} else {
					if( stack ) {
						img[y][x++] += p1;
						img[y][x++] += p2;
					} else {
						img[y][x++] = p1;
						img[y][x++] = p2;
					}
				}
			}
			fseek(f,12+16,SEEK_CUR);
		}
		stack_count++;
		while(  fread(header,1,4,f) == 4  ) {
			if( strncmp( header,"BRCM",4) == 0 ) {
				printf("next\n");
				fseek(f, -4 , SEEK_CUR );
				return 1;
			}
		}
		fclose(f);
		f = NULL;
		return 0;
}



void write_file( const char *fname , int rgb , int channel  )
{	
	FILE *out;
	int x,y;
	int xo = xmin + raw_xo;
	int yo = ymin + raw_yo;
	out = fopen(fname,"w");
	if(!out)
		exit(1);
		if(rgb)
			fprintf(out,"P6\n#offset %d %d\n%d %d\n%d\n",xo,yo,xmax-xmin,ymax-ymin,max);
		else
			fprintf(out,"P5\n#offset %d %d\n%d %d\n%d\n",xo,yo,xmax-xmin,ymax-ymin,max);
		fflush(out);
		for(y=ymin;y<ymax;y++) {
			for(x=xmin;x<xmax;x++) {
				int r,g1,g2,b;
				b = img[y*2][x*2];
				g1 = img[y*2+1][x*2];
				g2 = img[y*2][x*2+1];
				r = img[y*2+1][x*2+1];
				if( rgb ) {
					fputc( r/256 , out );
					if(max>255)
						fputc( r&255 , out );
				}
				if(channel == 0 )
					g1 = r;
				if(channel == 2 )
					g1 = g2;
				if(channel == 3 )
					g1 = b;
				fputc( g1/256 , out );
				if(max>255)
					fputc( g1&255 , out );
				if( rgb ) {
					fputc( b/256 , out );
					if(max>255)
						fputc( b&255 , out );
				}
			}

		}
		fclose(out);
}


int main( int argc,char **argv )
{
	FILE *out;
	int i;
	int y;
	int x;
	int nowrite=0;
    	char newest[256] = {0};
	char fname[1024];
	struct utimbuf utbuf; 
	if( argc == 1 ) {
		fprintf(stderr,"usage: readraw -nowrite -rgb -16 -scale16 -autocrop -crop xmin ymin xmax ymax -prefix prefix file1.raw file2.jpg\n"),
		exit(1);

	}
	while( argc >= 2 && argv[1][0] == '-' )  {
		if( strcmp( argv[1] , "-rgb" ) == 0 ) 
			rgb = 1;

		if( strcmp( argv[1] , "-16" ) == 0 )
			max = 65535;
		if( strcmp( argv[1] , "-8" ) == 0 )
			max = 255;
		if( strcmp( argv[1] , "-scale2" ) == 0 )
			scale = 2 ;
		if( strcmp( argv[1] , "-scale4" ) == 0 )
			scale = 4 ;
		if( strcmp( argv[1] , "-scale16" ) == 0 )
			scale = 16 ;
		if( strcmp( argv[1] , "-scale64" ) == 0 )
			scale = 64 ;
		if( strcmp( argv[1] , "-nowrite" ) == 0 )
			nowrite = 1  ;
		if( strcmp( argv[1] , "-stack" ) == 0 )
			stack = 1  ;
		if( strcmp( argv[1] , "-autocrop" ) == 0 ) {
			autocrop = 1;
		}
		if( strcmp( argv[1] , "-oddy" ) == 0 ) {
			oddy=1;
		}
		if( strcmp( argv[1] , "-crop" ) == 0 ) {
			xmin= atoi(argv[2]) ;
			ymin= atoi(argv[3]) ;
			xmax= atoi(argv[4]) ;
			ymax= atoi(argv[5]) ;
			argc -= 4;
			argv += 4;
		}
		if( strcmp( argv[1] , "-prefix" ) == 0 ) {
			strcpy(prefix,argv[2]);
			argc--;
			argv++;
		}
		if( strcmp( argv[1] , "-blacklevel" ) == 0 ) {
			blacklevel = atoi(argv[2]);
			argc--;
			argv++;
		}
		if( strcmp( argv[1] , "-zoom" ) == 0 ) {
			printf("init_screen\n");
			init_screen();
			printf("init_screen done\n");
			zoom=4;
		}
		argc--;
		argv++;
	}
	printf("scale = %d\n",scale);
	if( zoom ) {
		while(1) {
			int ret = read_dir(newest);
			if( ret == 0 ) {
				read_raw( newest );
				find_max(&x_maxval,&y_maxval);
				x_maxval/=2;
				y_maxval/=2;
				showpic(img,x_maxval-32,y_maxval-32,zoom);
			}
			sleep(1);
		}
	}

	for( i=1 ; i< argc;i++ ) {
    		struct stat st;
		int xm,ym;
		int ret ;
		const char *fname_in = argv[i];
		stat( argv[i], &st );
		utbuf.actime = st.st_atim.tv_sec; 
		utbuf.modtime = st.st_mtim.tv_sec; 
		fprintf(stderr,"reading %s\n",argv[i]);
		do {
			ret = read_raw( fname_in );
			if( ret < 0 )
				break;
			printf("read_raw done\n");
			find_max(&xm,&ym);
			find_data_range();
			printf("xm %d ym %d\n",xm/2,ym/2);
			printf("data range %d %d %d %d\n",x_min/2 - 100,y_min/2 - 100,x_max/2 + 100,y_max/2 + 100 );
			if( autocrop ) {
				xmin = xm/2 - 100;
				ymin = ym/2 - 100;
				xmax = xm/2 + 100;
				ymax = ym/2 + 100;
			}
			if(nowrite || stack )
				continue;
			if( rgb ) {
				sprintf(fname,"%s_%06d.ppm",prefix,idx++);
				write_file(fname,1,1);
				utime(fname,&utbuf );
			} else {
				sprintf(fname,"%s_%06d_r1.pgm",prefix,idx);
				write_file(fname,0,0);
				utime(fname,&utbuf );
				sprintf(fname,"%s_%06d_g1.pgm",prefix,idx);
				write_file(fname,0,1);
				utime(fname,&utbuf );
				sprintf(fname,"%s_%06d_g2.pgm",prefix,idx);
				write_file(fname,0,2);
				utime(fname,&utbuf );
				sprintf(fname,"%s_%06d_b1.pgm",prefix,idx);
				write_file(fname,0,3);
				utime(fname,&utbuf );
				idx++;
			}
			fname_in = NULL;
		} while( ret );

	}
	if( stack ) {
		int x,y;
		for(y=0;y<3056;y++)
			for(x=0;x<4056;x++)
				img[y][x]/=stack_count;
		if( rgb ) {
			sprintf(fname,"%s_stack.ppm",prefix);
			write_file(fname,1,1);
			utime(fname,&utbuf );
		} else {
			sprintf(fname,"%s_r1_stack.pgm",prefix);
			write_file(fname,0,0);
			utime(fname,&utbuf );
			sprintf(fname,"%s_g1_stack.pgm",prefix);
			write_file(fname,0,1);
			utime(fname,&utbuf );
			sprintf(fname,"%s_g2_stack.pgm",prefix);
			write_file(fname,0,2);
			utime(fname,&utbuf );
			sprintf(fname,"%s_b1_stack.pgm",prefix);
			write_file(fname,0,3);
			utime(fname,&utbuf );
			idx++;
		}
			


	}

}
	


