#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void do_some_work() {
  printf("Now I am going to do some work...\n");
  long int i;
  long int j = 0;
  for(i = 0; i < 3000000000; i++) {
    j += 3;
  }
  printf("Work done! My result j = %ld\n", j);
}

int main(int argc, char** argv) {

  if (argc < 2) {
      printf("Usage: %s <num_threads>\n", argv[0]);
      return 1;
  }
  int n = atoi(argv[1]);
  if (n <= 0) {
    printf("Error: num_threads must be > 0\n");
    return 1;
  }

  double t0 = omp_get_wtime();  
  #pragma omp parallel num_threads(n)
  {
    do_some_work();
  }
  double t1 = omp_get_wtime();
  printf("Wall time = %.6f seconds\n", t1 - t0);
  return 0;
}
