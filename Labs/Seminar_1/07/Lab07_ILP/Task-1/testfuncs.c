#include "testfuncs.h"

void f_std(const double * __restrict a, 
	   const double * __restrict b, 
	   double * __restrict c, int N) {
  int i;
  double x = 0.3;
  for(i = 0; i < N; i++) {
    c[i] = a[i]*a[i] + b[i] + x;
  }
}

void f_opt(const double * __restrict a, 
	   const double * __restrict b, 
	   double * __restrict c, int N) {
  int i = 0;
  double x = 0.3;
  /*// unroll by 4
  for (; i + 3 < N; i += 4) {
    double a0 = a[i];
    double a1 = a[i+1];
    double a2 = a[i+2];
    double a3 = a[i+3];
    c[i]   = a0*a0 + b[i]   + x;
    c[i+1] = a1*a1 + b[i+1] + x;
    c[i+2] = a2*a2 + b[i+2] + x;
    c[i+3] = a3*a3 + b[i+3] + x;
  }*/
  // unroll by 3
  for (; i + 2 < N; i += 3) {
    double a0 = a[i];
    double a1 = a[i+1];
    double a2 = a[i+2];

    c[i]   = a0*a0 + b[i]   + x;
    c[i+1] = a1*a1 + b[i+1] + x;
    c[i+2] = a2*a2 + b[i+2] + x;
  }
  for(i = 0; i < N; i++) {
    c[i] = a[i]*a[i] + b[i] + x;
  }
}

