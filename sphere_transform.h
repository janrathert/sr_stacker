
typedef struct {
	double A[3][3];
	double M[3][3];
	double b[3];
} sphere_matrix;

	



int set_matrix3( float x1 ,float y1 , float x2 , float y2 , float center_x , float center_y , float r , sphere_matrix *m , sphere_matrix *m0  );
void map_point3( sphere_matrix *m , sphere_matrix *m0,  int x , int y , float *xx , float *yy );

