#include <stdio.h>
#include <math.h>

#include "sphere_transform.h"
	

void matrix_mult_3( double R[3][3] , double A[3][3] , double B[3][3] ) // R = A*B
{
	int i;
	int j;
	for(i=0;i<3;i++)
		for(j=0;j<3;j++)
			R[i][j] = A[i][0]*B[0][j] + A[i][1] * B[1][j] + A[i][2] * B[2][j] ;
}

void matrix_mult_vector_3( double r[3], double A[3][3] , double b0 , double b1 , double b2 ) // r = A*b
{
	int i;
	for(i=0;i<3;i++)
		r[i] = A[i][0]*b0 + A[i][1] * b1 + A[i][2] * b2; 
}

void matrix_transpose_3( double R[3][3] , double A[3][3] )
{
	int i,j;
	for(i=0;i<3;i++)
		for(j=0;j<3;j++)
			R[i][j] = A[j][i];
}




int set_matrix3( float x1 ,float y1 , float x2 , float y2 , float center_x , float center_y , float r , sphere_matrix *m , sphere_matrix *m0  )
{
	double T[3][3];

	double r1;
	double r2;

	double ax = x1-center_x;
	double ay = y1-center_y;
	double az = r;
	double bx = x2-center_x;
	double by = y2-center_y;
	double bz = r;
	double cx,cy,cz;
	r1 = sqrt( ax*ax + ay*ay + az*az );
	ax /= r1;
	ay /= r1;
	az /= r1;

	double db = ax*bx + ay*by + az*bz;
	bx -= db*ax;
	by -= db*ay;
	bz -= db*az;
	r2 = sqrt( bx*bx + by*by + bz*bz );
	bx /= r2;
	by /= r2;
	bz /= r2;


	cx = ay*bz - az*by ;
	cy = az*bx - ax*bz ;
	cz = ax*by - ay*bx ;

	// printf("%f %f %f\n",ax,ay,az );
	// printf("%f %f %f\n",bx,by,bz );
	// printf("%f %f %f\n",cx,cy,cz );

	m->A[0][0] = bx;
	m->A[1][0] = by;
	m->A[2][0] = bz;
	m->A[0][1] = cx;
	m->A[1][1] = cy;
	m->A[2][1] = cz;
	m->A[0][2] = ax;
	m->A[1][2] = ay;
	m->A[2][2] = az;
	m->b[0] = center_x;
	m->b[1] = center_y;
	m->b[2] = -r;
	matrix_transpose_3(T,m->A );
	matrix_mult_3(m->M,m0->A,T);


}

void map_point3( sphere_matrix *m , sphere_matrix *m0 , int x , int y , float *xx , float *yy )
{

	// m0->A * m->A^T * p 
	double x_ = x - m->b[0];
	double y_ = y - m->b[1];
	double z_ = -m->b[2];
	double v[3];
	matrix_mult_vector_3(v,m->M,x_,y_,z_ );
	v[0]/=v[2];
	v[1]/=v[2];
	v[0] *= -m0->b[2];
	v[1] *= -m0->b[2];
	v[0] += m0->b[0];
	v[1] += m0->b[1];
	*xx = v[0];
	*yy = v[1];
}

#ifdef TESTMAIN 
int main( )
{
	float x1 = 100;
	float y1 = 100;
	float x2 = 200;
	float y2 = 100;
	float cx = 100;
	float cy = 100;
	float r = 200;
	float ox;
	float oy;
	int x = 100;
	int y = 100;
	sphere_matrix m1;
	set_matrix3( x1 ,y1 , x2 , y2 , cx , cy ,  r , &m1  );
	map_point3( &m1 , &m1 , x , y , &ox , &oy );
	printf("ox = %f oy = %f\n",ox,oy );
	


}
#endif
