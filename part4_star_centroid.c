#include <stdio.h>
#include <math.h>
 
void calcSubPixelCenter( unsigned short *img , int w, int h , float * outX, float * outY,
                         int inNumIter /*num iterations*/ , int x, int y) { // 10 iter
  // Sub pixel interpolation
  float c, a1, a2, a3, a4, b1, b2, b3, b4;
  float a1n, a2n, a3n, a4n, b1n, b2n, b3n, b4n;

  img += y*w+x;

  b1 = img[0];
  a2 = img[1];
  b2 = img[2];
  img += w; 
  a1 = img[0];
  c = img[1];
  a3 = img[2];
  img += w; 
  b4 = img[0];
  a4 = img[1];
  b3 = img[2];
 
  // b1 = inImg(0, 0); a2 = inImg(1, 0); b2 = inImg(2, 0);
  // a1 = inImg(0, 1);  c = inImg(1, 1); a3 = inImg(2, 1);
  // b4 = inImg(0, 2); a4 = inImg(1, 2); b3 = inImg(2, 2);
 
  for (size_t i = 0; i < inNumIter; ++i) {
    float c2 = 2 * c;
    float sp1 = (a1 + a2 + c2) / 4;
    float sp2 = (a2 + a3 + c2) / 4;
    float sp3 = (a3 + a4 + c2) / 4;
    float sp4 = (a4 + a1 + c2) / 4;
    
    // New maximum is center
    // float newC = std::max({ sp1, sp2, sp3, sp4 });
    float newC = sp1;
    if( sp2 > newC )
		newC = sp2;
    if( sp3 > newC )
		newC = sp3;
    if( sp4 > newC )
		newC = sp4;
    
    // Calc position of new center
    float ad = pow(2.0, -((float) i + 1));
 
    if (newC == sp1) {
      *outX = *outX - ad; // to the left
      *outY = *outY - ad; // to the top
 
      // Calculate new sub pixel values
      b1n = (a1 + a2 + 2 * b1) / 4;
      b2n = (c + b2 + 2 * a2) / 4;
      b3n = sp3;
      b4n = (b4 + c + 2 * a1) / 4;
      a1n = (b1n + c + 2 * a1) / 4;
      a2n = (b1n + c + 2 * a2) / 4;
      a3n = sp2;
      a4n = sp4;
 
    } else if (newC == sp2) {
      *outX = *outX + ad; // to the right
      *outY = *outY - ad; // to the top
 
      // Calculate new sub pixel values
      b1n = (2 * a2 + b1 + c) / 4;
      b2n = (2 * b2 + a3 + a2) / 4;
      b3n = (2 * a3 + b3 + c) / 4;
      b4n = sp4;
      a1n = sp1;
      a2n = (b2n + c + 2 * a2) / 4;
      a3n = (b2n + c + 2 * a3) / 4;
      a4n = sp3;
    } else if (newC == sp3) {
      *outX = *outX + ad; // to the right
      *outY = *outY + ad; // to the bottom
 
      // Calculate new sub pixel values
      b1n = sp1;
      b2n = (b2 + 2 * a3 + c) / 4;
      b3n = (2 * b3 + a3 + a4) / 4;
      b4n = (2 * a4 + b4 + c) / 4;
      a1n = sp4;
      a2n = sp2;
      a3n = (b3n + 2 * a3 + c) / 4;
      a4n = (b3n + 2 * a4 + c) / 4;
    } else {
      *outX = *outX - ad; // to the left
      *outY = *outY + ad; // to the bottom   
 
      // Calculate new sub pixel values
      b1n = (2 * a1 + b1 + c) / 4;
      b2n = sp2;
      b3n = (c + b3 + 2 * a4) / 4;
      b4n = (2 * b4 + a1 + a4) / 4;
      a1n = (b4n + 2 * a1 + c) / 4;
      a2n = sp1;
      a3n = sp3;
      a4n = (b4n + 2 * a4 + c) / 4;
    }
 
    c = newC; // Oi = Oi+1
 
    a1 = a1n;
    a2 = a2n;
    a3 = a3n;
    a4 = a4n;
 
    b1 = b1n;
    b2 = b2n;
    b3 = b3n;
    b4 = b4n;
  }
}

#ifdef TESTMAIN

int main()
{
	unsigned short img[] = {
	  80 ,     90 ,     89 , 
     87 ,    113 ,    110 , 
     93 ,    108 ,    110  };
	float ox = 0 , oy = 0;

	calcSubPixelCenter( img , 3, 3 , &ox , &oy , 10 , 0 , 0 );
	printf("%f %f\n",ox,oy );

}
#endif
