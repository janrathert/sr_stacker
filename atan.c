
#include <stdio.h>
#include <math.h>
// hq camera chip size 6.287mm x 4.712mm

int main()
{
	float jupiter_radius = 70000;
	float jupiter_distance = 3.98 * 149597870.0     ; 

	printf(" jupiter diameter pixels %f\n", (jupiter_radius*2) / jupiter_distance * 180  / (6.287/4056) );
	printf(" moon diameter pixels %f\n", 1700*2 / 300000.0 * 180  / (6.287/4056) );
	printf(" artificial star diameter pixels %f\n", 1 / 8000.0 * 180  / (6.287/4056) );
	printf("%f\n",atan(6.287/2/182)*180 * 2  );
	printf("%f\n",atan(6.287/2/182/8)*180 * 2  );
	printf("%f\n",atan(6.287/4056/2/182)*180 * 2 * 3600  );

}
