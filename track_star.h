float get_center_of_light( unsigned short *v , int w , int h , int x_ , int y_ , int r ,  float *cx , float *cy , float black_level , int *count );
float get_max_point( unsigned short *v , int w , int h , int x_ , int y_ , int r ,  float *cx , float *cy  );
int get_psf( unsigned short *v , int w , int h , float cx , float cy , int r , float *fwhm , const char *fname   );
int get_psf3( unsigned short *v , int w , int h , float cx , float cy , int r , float blacklevel , const char *fname   );
unsigned short *get_sub_image( unsigned short *v , int w , int h , float cx , float cy , int r , float pixelstep  );
unsigned short *get_sub_image_ext( unsigned short *v , int w , int h , float cx , float cy , int r , float pixelstep  );
long long simple_image_diff( unsigned short *img1 , unsigned short *img2 , int w,int h);
long long cross_correlation( unsigned short *img1 , unsigned short *img2 , int w,int h);
long long fuzzy_image_diff( unsigned short *needle[16][16] , unsigned short *img2 , int w,int h , unsigned char *offsets );
unsigned short *downsample( unsigned short *img , int w, int h , int x , int y , int w_ , int h_  , int scale  );
