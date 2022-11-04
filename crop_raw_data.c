

static int find_max( unsigned char *buf , int *xmax_ , int *ymax_ )
{
		int max = 0;
		int xmax;
		int ymax;
		int x,y;
		for(y=0;y<3056;y++) {
			int j;
			
			for(j=0,x=0;x<4056;j+=3,x+=2) {
				if( buf[j]>max) {
					max = buf[j];
					xmax = x;
					ymax = y;
				}
					
			}
			buf += (4056*12)/8 + 12 + 16 ;
		}
		*xmax_ = xmax;
		*ymax_ = ymax;
		return max;
}

int crop_image_data( unsigned char *buf, FILE *f , int w , int h )
{
		int max = 0;
		int xmax;
		int ymax;
		int y0;
		int y1;
		int x0;
		find_max(buf,&xmax,&ymax);
		y0 = ymax-h/2;
		y1 = ymax+h/2;
		x0 = xmax-w/2;
		x1 = xmax+w/2;
		for(y=y0;y<y1;y++) {
			int j;
			fwrite(buf+(x0/2)*3,3,(w/2)*3,f);
			
			buf += (4056*12)/8 + 12 + 16 ;
		}
}
