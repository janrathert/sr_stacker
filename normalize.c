#include <math.h>
#include <stdio.h>
#include <stdint.h>

int normalize_img_avg_mean_deviation( valtype *img , int w ,int h , double *avg_ , double *mean_deviation_ , double new_avg , double new_mean_deviation  )
{
	int i;
	int size = w*h;
	double sum = 0;
	float avg;
	float mean_deviation = 0;
	for(i=0;i<size;i++)
		sum += img[i];
	avg = sum/size;
	if( new_avg >= 65535 ) {
		avg = 0;
		new_avg = 0;
	}
	for(i=0;i<size;i++)
		mean_deviation +=  (img[i]-avg)*(img[i]-avg);
	mean_deviation = sqrt( mean_deviation / size );

	if( avg_ )
		*avg_ = avg;
	if( mean_deviation_ )
		*mean_deviation_ = mean_deviation;
	// printf("avg = %f new_avg = %f sigma = %f new_sigma = %f %f\n",avg,new_avg,mean_deviation,new_mean_deviation,new_mean_deviation/mean_deviation );

	if( !new_mean_deviation )
		return 0;

	for(i=0;i<size;i++) {
		img[i] = ( img[i] - avg ) * new_mean_deviation/mean_deviation + new_avg;
		// if( img[i] < -60000 || img[i]>5*60000 )
		//	printf("bad image value after normalize\n");
	}

	return 0;
}


int normalize_img_hist( valtype *img , int w ,int h , double *val1 , double *val2 , double new_val1 , double new_val2  )
{
	int size = w*h;
	valtype sum = 0;
	float avg;
	float mean_deviation = 0;
	int hist[65536] = {0};
	int i;
	int j;
	int k;
	int highest[10];
	for(i=0;i<size;i++) {
		j = img[i];
		if( j < 0 )
			j = 0;
		if( j > 65535 )
			j = 65535;
		hist[j]++;
	}
	j = 0;
	for(i=65535;i>=0 && j<9;i--) {
		if( hist[i] )  {
			for(k=0;k<hist[i] && j<9;k++,j++)
				highest[j] = i;
		}
	}
	printf("highest\n");
	for(i=0;i<9;i++)
		printf("%d ",highest[i]);
	printf("\n");
	
	if( val2 )
		*val2 = highest[4];
		
	if( !new_val2 )
		return 0;
	for(i=0;i<size;i++) {
		img[i] = ( img[i] * new_val2 )/highest[4] ;
	}
				

	return 0;
}

int normalize_img( valtype *img , int w ,int h , double *val1 , double *val2 , double new_val1 , double new_val2  )
{
	// return normalize_img_hist(img,w,h, val1,val2, new_val1,new_val2);
	return normalize_img_avg_mean_deviation(img,w,h, val1,val2, new_val1,new_val2);
}
