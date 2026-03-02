#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

  if(argc != 2) {
    printf("Please give 1 arg: number of threads to use.\n");
    return -1;
  }
  int nThreads = atoi(argv[1]);

  int i;
  const int n = 1000000;
  double globsum;
  double* A;
  A = (double*)malloc(n*sizeof(double));

  for (i = 0; i < n ; i++) { 
    A[i]=(double)i;
  }

  int repeat;
  for(repeat = 0; repeat < 400; repeat++) {

    globsum=0.0;

#pragma omp parallel for num_threads(nThreads) reduction(+:globsum)
    for (i = 0; i < n ; i++) {
      globsum += A[i];
    }

  printf("Global sum is: %f\n",  globsum);

  free(A);

  return 0;
}
}