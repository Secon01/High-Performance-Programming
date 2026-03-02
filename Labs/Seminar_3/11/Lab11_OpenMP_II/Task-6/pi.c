/**********************************************************************
 * This program calculates pi using C
 *
 **********************************************************************/
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

  int i;
  const int intervals = 500000000;
  double sum, dx, x;

  dx  = 1.0/intervals;
  sum = 0.0;
  int nThreads = (argc > 1) ? atoi(argv[1]) : omp_get_max_threads();
  omp_set_num_threads(nThreads);

#pragma omp parallel for private(x) reduction(+:sum)
  for (i = 1; i <= intervals; i++) {
    x = dx*(i - 0.5);
    sum += dx*4.0/(1.0 + x*x);
  }
  printf("PI is approx. %.16f\n",  sum);

  return 0;
}
