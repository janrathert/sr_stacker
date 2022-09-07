#include <stdio.h>
#include <math.h>
#include <string.h>

static int getpixel_ushort_( unsigned short *v , int w , int h , int x , int y  )
{
	int a,b,c,d;
	int e,f,g;
	if( y < 0 || x < 0 || x >= w-1 || y >= h-1 ) 
		return 0;
	v += w*y+x;
	return *v;
}

static int getpixel_ushort( unsigned short *v , int w , int h , int x , int y , int xsub , int ysub )
{
	int a,b,c,d;
	int e,f,g;
	if( y < 0 || x < 0 || x >= w-1 || y >= h-1 ) 
		return 0;
	v += w*y+x;
	a = *v;
	b = v[1];
	c = v[w];
	d = v[w+1];

	e = ( b*xsub + (16-xsub)*a)/16;
	f = ( d*xsub + (16-xsub)*c)/16;
	g = ( f*ysub + (16-ysub)*e)/16;
	/*
	if( g == 5376 )
		printf("a=%d b=%d c=%d d=%d x=%d y=%d\n",a,b,c,d,x,y);
	*/

	
	return g;
}

int getpixel_sum( unsigned short *v , int w, int h , int x, int y , int w_ , int h_ , int min , int max    )
{
	int sum = 0;
	int i,j;
	v += y*w + x;
	for(i=0;i<h_;i++) {
		int j;
		for(j=0;j<w_;j++) {
			int val = (*v++);
			if( val<min)
				val = min;
			if( val>max)
				val = max;
			sum += val ;
		}
		v += w - w_;
	}
	return sum;

}

static int in_range_of_4( unsigned short *img , int w, int h , int v )
{
	int v0,v1,v2,v3;
	v0 = *img;
	v1 = img[1];
	v2 = img[w];
	v3 = img[w+1];
	if( v < v0 && v<v1 && v<v2 && v<v3 )
		return 0;
	if( v > v0 && v>v1 && v>v2 && v>v3 )
		return 0;
	return 1;
}


long long image_diff_range( unsigned short *img1 , unsigned short *img2 , int w1,int h1, int w2, int h2, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h,
			int min1 , int max1 , int min2 , int max2 )
{
	int w_ ;
	int penalty = 0;
	int x,y;
	img1 += y1*w1 + x1;
	img2 += y2*w2 + x2;
	for( y = 0 ; y  < cmp_h ; y++ ) 
		for( x = 0 ; x  <  cmp_w ; x++  )  {
			penalty -= in_range_of_4( img2, w2 , h2, ((img1[y*w1+x]-min1)*(max2-min2))/(max1-min1)+min2 );
		}
	// printf("in range = %d/%d\n",-penalty , cmp_w*cmp_h );
	penalty += cmp_w*cmp_h;
	return penalty;

}



long long image_diff( unsigned short *img1 , unsigned short *img2 , int w1,int h1, int w2, int h2, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h,
			int avg1, int avg2  )
{
	long long diff = 0;
	int i;
	img1 += y1*w1 + x1;
	img2 += y2*w2 + x2;
	if(!avg1)
		return 10000;
	for(i=0;i<cmp_h;i++) {
		int j;
		for(j=0;j<cmp_w;j++) {
			long long d = (*img1++) - (((*img2++)*(long long)avg2))/avg1;
			// long long d = (*img1++) - (*img2++);
			diff += d*d;
		}
		img1 += w1 - cmp_w;
		img2 += w2 - cmp_w;
	}
	return diff;
}

long long image_diff_bounds( unsigned short *img1 , unsigned short *img2 , int w1,int h1, int w2, int h2, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h,
			int avg1 , int avg2 , int min1 , int max1 , int min2 , int max2 )
{
	int w_ ;
	long long penalty = 0;
	int x,y;
	int min1_,min2_;
	int max1_,max2_;
	img1 += y1*w1 + x1;
	img2 += y2*w2 + x2;
 	return	 image_diff( img1, img2 , w1,h1,w2,h2,0,0,0,0,cmp_w,cmp_h,avg1,avg2 );
	min1_ = ( 32768LL * min1 ) / avg1 ; 
	max1_ = ( 32768LL * max1 ) / avg1 ; 
	min2_ = ( 32768LL * min1 ) / avg2 ; 
	max2_ = ( 32768LL * max1 ) / avg2 ; 
	for( w_ = 1 ; w_ <= 8; w_++  ) {
		for( y = 0 ; y + w_ <= cmp_h ; y++ ) 
			for( x = 0 ; x + w_ <= cmp_w ; x++  )  {
				int sum1 = getpixel_sum( img1, w1,h1 , x,y, w_ , w_ , min1 , max1   );
				int sum2 = getpixel_sum( img2, w2,h2 , x,y, w_+1 , w_+1 , min2 , max2   );
				sum1 = ( 32768LL * sum1 ) / avg1 ; 
				sum2 = ( 32768LL * sum2 ) / avg2 ; 
				if( sum1 - min1_*w_*w_   > sum2 - min2_*(w_+1)*(w_+1)  )
					penalty += w_*w_;
				if( max1_*w_*w_ - sum1 > max2_*(w_+1)*(w_+1) - sum2 )
					penalty += w_*w_;

			}


	}
	if( penalty == 0 ) {
		penalty = image_diff( img1, img2 , w1,h1,w2,h2,0,0,0,0,cmp_w,cmp_h,avg1,avg2 );
		// printf("image_diff = %lld\n",penalty);
	} else
		penalty += 1000000000LL;

	return penalty;
}


long long image_diff_subpixel( unsigned short *img1 , unsigned short *img2 , int w, int h, int x1, int y1 , int x2 , int y2 , int cmp_w , int cmp_h , int x2_sub , int y2_sub,
				int avg1 , int avg2 )
{
	long long diff = 0;
	int i;
	for(i=0;i<cmp_h;i++) {
		int j;
		for(j=0;j<cmp_w;j++) {
			int v1;
			int v2;
			v1 = getpixel_ushort(img1,w,h,x1+j,y1+i,0,0);
			v2 = getpixel_ushort(img2,w,h,x2+j,y2+i,x2_sub,y2_sub);
			long long d = v2-(v1*(long long)avg2)/avg1 ;
			// long long d = v2-v1 ;
			diff += d*d;
		}
	}
	return diff;
}


long long best_image_match( unsigned short *needle, unsigned short *haystack , int w1,int h1, int w2, int h2 ,  int *xmatch, int *ymatch )
{	
	long long min = -1 ;
	int x2,y2;
	int xm,ym;
	for(y2=0;y2<h2-h1;y2++) {
		for(x2=0;x2<w2-w1;x2++) {
			long long diff;
			if( getpixel_ushort_(haystack,w2,h2, x2,y2 ) < 1000 )
				continue;
			diff = image_diff(needle,haystack,w1,h1,w2,h2, 0,0 , x2,y2, w1,h1 , 1 , 1  );
			if(min == -1 || diff < min ) {
				min = diff;
				*xmatch = xm = x2;
				*ymatch = ym = y2;
			}
		}
	}
	return min;
}



