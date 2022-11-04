#include<stdio.h>
#include<math.h>
#include "gauss_jordan.h"


int fit_circle( float *xp , float *yp , int n , float *cx , float *cy , float *r )
{
	float M[n][3];
	float B[3][4];
	float y[n];
	float *B_[3];
	float z[n];
	int i;
	int j;
	int k;
	if( n < 3 )
		return -1;
	for(i=0;i<n;i++) {
		M[i][0] = xp[i];
		M[i][1] = yp[i];
		M[i][2] = 1;
		y[i] = xp[i]*xp[i] + yp[i]*yp[i]; 
	}
	for(i=0;i<3;i++) {
		for(j=0;j<3;j++) {
			float sum = 0;
			for(k=0;k<n;k++)
				sum += M[k][i] * M[k][j];
			B[i][j] = sum;
		}
	}
	for(i=0;i<3;i++) {
		float sum = 0;
		for(j=0;j<n;j++)
			sum += M[j][i] * y[j];
		B[i][3] = sum;
	}
	for(i=0;i<3;i++)
		B_[i] = B[i];
	/*
	for(i=0;i<3;i++) {
		for(j=0;j<3+1;j++) {
			printf("%5.5f ",B_[i][j] );
		}
		printf("\n");
	}
	*/
	if( gauss_jordan(B_,z,3) < 0 )
		return -1;
	/*
	for(i=0;i<3;i++) {
		printf("z %f\n",z[i] );
	}
	*/
	*cx = z[0]*0.5;
	*cy = z[1]*0.5;
	*r = sqrt(z[2] + (*cx)*(*cx) + (*cy)*(*cy)  );
	return 0;

}

#ifdef TESTMAIN_CIRCLE
int main()
{
	int xp[] = {0,1,2,1};
	int yp[] = {0,0,0,1};
	float cx,cy,r;
	fit_circle(xp,yp,3,&cy,&cy,&r);

}
#endif

