#include <stdio.h>
#include <math.h>



float gauss_distribution( float x , float ro ) //  ro = standard
{
	return 1 / sqrt(2*M_PI*ro*ro ) * exp( -x*x / (2*ro*ro) );
}

#ifdef XPLOT

int main(int argc,char **argv)
{
	FILE *f;	
	float ro;
	float x;
	float max;
	sscanf(argv[1],"%f",&ro );
	max = atoi(argv[2])/gauss_distribution( 0 , ro );
	printf("%f %f\n", gauss_distribution( 0 , ro ) ,  gauss_distribution( ro*2.355/2 , ro ) );
	printf("%f\n", ( ro*2.355 ) );
	f = fopen("xplot.txt","w");
	for(x=0;x<5;x+=1) {
		fprintf(f,"%f %f\n",x,max*gauss_distribution( x , ro ) );
		printf("%f %f\n",x,max*gauss_distribution( x , ro ) );
	}
	fclose(f);

}
#endif

float gauss_distribution2d( float x,float y, float x0 , float y0 , float ro )
{
	x -= x0;
	y -= y0;
	x = sqrt( x*x + y*y );
	return gauss_distribution( x , ro );
}

static float intersect( float x1,float x2 , float  x1_ , float x2_ )
{
	if( x2_ < x1 || x2 < x1_ || x1_ > x2 || x1 > x2_  )
		return 0;
	if( x1_ >= x1 && x2_ <= x2 )
		return x2_ - x1_ ;
	if( x1 >= x1_ && x2 <= x2_ )
		return x2 - x1 ;
	if( x2 >= x2_ )
		return x2_ - x1;
	if( x2_ >= x2 )
		return x2 - x1_;
	if( x1 <= x1_ )
		return x2 - x1_;
	if( x1_ <= x1 )
		return x2_ - x1;
	printf("must not happen\n");
	exit(1);
	return 0;
}

float rect_distribution( float x,float y, float x0 , float y0 , float ro )
{
	float ret = intersect( x-0.5,x+0.5 , x0-ro , x0+ro ) * 
	       intersect( y-0.5,y+0.5 , y0-ro , y0+ro );  
	return ret;
}

#if 0
static float gauss_table[256][256];
void gen_gauss_table( float ro )
{
	int i,j;
	int w=256;
	for(i=0;i<w;i++) {
		for(j=0;j<w;j++) {
			gauss_table[i][j] = gauss_distribution2d( i/16.0,j/16.0,w/2/16.0,w/2/16.0, ro );
		}
	}
}

float gauss_distribution2d_int( float x,float y, float x0 , float y0 , float ro )
{
	float sum = 0;
	x -= x0;
	y -= y0;
	x*=16;
	y*=16;
	x+=128;
	y+=128;

	x = sqrt( x*x + y*y );
	return gauss_distribution( x , ro );
}
#endif


#ifdef TESTMAIN 
int main()
{
	FILE *f;
	int i,j;
	int w=99;
	int max = 0;
	printf("P5\n%d %d\n65535\n",w,w);
	for(i=0;i<w;i++) {
		for(j=0;j<w;j++) {
			int v = gauss_distribution2d( i/10.0,j/10.0,w/2/10.0,w/2/10.0, 2 ) *  50000/0.3  ;
			if( v>max)
				max = v;
			fputc(v/256,stdout);
			fputc(v&255,stdout);
		}
	}
	fprintf(stderr,"max=%d\n",max);
}
#endif
