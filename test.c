#include <stdio.h>
#include <math.h>
// hello world
int main()
{
	FILE *f;
	float dx;
	float dy;
	
				  f = fopen("offsets.txt","r");
				  if( f ) {
					char dummy[32];
					float dx,dy;
					fscanf(f,"%s %f %s %f",dummy,&dx,dummy,&dy );
					fclose(f);
					printf("dx = %f dy = %f\n",dx,dy);
					
				  }

}
