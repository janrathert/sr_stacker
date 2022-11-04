#include <stdio.h>
#include <math.h>
#include "gauss_distribution.h"

float x[100],y[100];
int n;
int i_fwhm;
float sigma , sigma_min ;
int zoom;

float compare_gauss( float *x , float *y , int n , float sigma )
{
	float max1 = y[0];
	float max2 = gauss_distribution( 0, sigma );
	int i;
	float sum = 0;
	for(i=0;i<n;i++) {
		float diff = (y[i] - gauss_distribution( x[i] , sigma ) / max2 * max1 );
		diff = pow(diff,2);
		sum += diff;
	}
	return sum;
	

}

float eval( float x_ )
{
	float a;
	float z;
	int i;
	if( 2*x_ < i_fwhm )
		return gauss_distribution(x_,sigma_min)*y[0]/gauss_distribution(0,sigma_min);
	i = floor( 2*x_ );
	if( i>= n-1 )
		return 0;
	a = (x_ - x[i])*2;
	return (1-a)*y[i] + a*y[i+1];
}

void genpic( int w )
{
	int i,j;
	int h=w/2;
	FILE *f;

	f = fopen("psf.pgm","w");
	fprintf(f,"P5\n%d %d\n%d\n",w,w,65535);
	for(i=0;i<w;i++) {
		for(j=0;j<w;j++) {
			float r = sqrt( (i-h)*(i-h)+(j-h)*(j-h) ); 
			int val = eval( r/zoom );
			fputc( val/256 , f);
			fputc( val&255 , f);

		}
	}
	fclose(f);

}

int main( int argc, char **argv )
{
	int i = 0;
	int j = 0;
	float min = 0;
	float gmax;
	float black;
	float step = 0.01;
	zoom = atoi(argv[1]);
	while( fscanf(stdin,"%f %f",&x[i],&y[i] ) == 2 ) {
		i++;
	}
	n = i;
	black = y[n-1];
	for(i=0;i<n;i++) {
		y[i] -= black;
		if( y[i] < y[0]/2 && !j )
			j = i;
	}
	i_fwhm = j;
	for ( sigma = 1 ; sigma < 10 ; sigma += step ) {
		float diff = compare_gauss( x,y,j , sigma );
		if( min == 0 || diff < min ) {
			min = diff;
			sigma_min = sigma;
		}
	}
	printf("sigma_min = %f\n",sigma_min );
	for(i=0;i<n*zoom;i++) {
		printf("%f %f\n",i*0.5, eval(i*0.5/zoom) );
	}
	genpic( (n*zoom)+1 );

}
