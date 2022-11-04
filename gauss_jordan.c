#include<stdio.h>


int gauss_jordan( float **A , float *x, int n)
{
    int i,j,k;
    float c;
    /* Now finding the elements of diagonal matrix */
    for(j=0; j<n; j++)
    {
        for(i=0; i<n; i++)
        {
            if(i!=j)
            {
		if( A[j][j]  == 0 ) {
			printf("bad matrix in gauss jordan\n");
			return -1;
		}
                c=A[i][j]/A[j][j];
                for(k=0; k<n+1; k++)
                {
                    A[i][k]=A[i][k]-c*A[j][k];
                }
            }
        }
    }
    for(i=0; i<n; i++)
    {
	if( A[i][i]  == 0 ) {
		printf("bad matrix in gauss jordan\n");
		return -1;
	}
        x[i]=A[i][n]/A[i][i];
    }
    return 0;
}

#ifdef TESTMAIN
int main()
{
	float A_[2][3] = {
		{1,2,3},
		{4,5,6}
	};
	float *A[2] = { A_[0] , A_[1] };
	float x[2];
	gauss_jordan(A,x,2);
	printf("%f %f\n",x[0],x[1] );

}
#endif
