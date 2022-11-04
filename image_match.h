
long long image_diff( unsigned short *img1 , unsigned short *img2 , int w1,int h1, int w2, int h2, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h , int avg1,int avg2 );
long long image_diff_subpixel( unsigned short *img1 , unsigned short *img2 , int w, int h, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h , int x2_sub , int y2_sub,
			int avg1, int avg2 );
long long best_image_match( unsigned short *needle, unsigned short *haystack , int w1, int h1 , int w2,int h2,   int *xmatch, int *ymatch );
long long image_diff_bounds( unsigned short *img1 , unsigned short *img2 , int w1,int h1, int w2, int h2, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h , int avg1, int avg2 , int min1, int max1 , int min2, int max2 );
long long image_diff_range( unsigned short *img1 , unsigned short *img2 , int w1,int h1, int w2, int h2, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h,
			int min1 , int max1 , int min2 , int max2 );
