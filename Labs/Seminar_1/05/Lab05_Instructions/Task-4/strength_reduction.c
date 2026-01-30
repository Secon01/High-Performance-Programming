#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
   int i;
   int a = 1, b = 2, c = 3, d = 1;
   float x = 0.1, y = 0.5, z = 0.33;
   printf("%d %d %d %d, %f %f %f\n", a, b, c, d, x, y, z);

   const float inv = 1.0f/1.33f;
   for (i=0; i<50000000; i++)
   {
      c = d << 2;             //c = d*2;
      b = (c << 4) - c;       //b = c*15;      
      a = b >> 4;             //a = b/16;
      d = b/a;

      z = 0.33f;              //z = 0.33;
      y = z + z;              //y = 2*z;
      x = y * inv;            //x = y / 1.33; 
      z = x * inv;            //z = x / 1.33;
   }
   printf("%d %d %d %d, %f %f %f\n", a, b, c, d, x, y, z);
   return 0;
}
