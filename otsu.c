#include <stdio.h>
#include <stdint.h>
#include <math.h>

int get_otsu_threshold( valtype *img , int w , int h , int x_ , int y_ , int r )
{
	int hist[65536] = {0};
	int i;
	float sum = 0;
	int x,y;
	printf("get_otsu_threshold %d %d   %d %d %d\n",w,h, x_,y_, r );
	for(y = y_-r;y<y_+r;y++) {
		for(x = x_-r;x<x_+r;x++) {
			int val = (img[y*w+x]+0.5);
			if( val < 0 )
				val = 0;
			if( val > 65535 )
				val = 65535;
			hist[ val ]++;
		}
	} 
	for( i=0;i<65536 ; i++ )
		sum += i*hist[i];
  	float numPixels = 4*r*r;
  	float sumB = 0, wB = 0, max = 0.0;
  	float threshold1 = 0.0, threshold2 = 0.0;
	for( i=0;i<65536 ; i++ ) {
    		wB += hist[i];
 
    		if (! wB) { continue; }    
 
    		float wF = numPixels - wB;
    
    		if (! wF) { break; }
    
    		sumB += i * hist[i];
 
		float mF = (sum - sumB) / wF;
		float mB = sumB / wB;
		float diff = mB - mF;
		float bw = wB * wF * pow(diff, 2.0);
    
	    	if (bw >= max) {
	      		threshold1 = i;
		        if (bw > max) {
				threshold2 = i;
		        }
		        max = bw;            
	    	}
	  } // end loop
  
  	  float th = (threshold1 + threshold2) / 2.0;

	return th;

}
 
