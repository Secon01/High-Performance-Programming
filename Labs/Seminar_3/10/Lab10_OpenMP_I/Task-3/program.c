#include <stdio.h>
#include <omp.h>

void thread_func() {
  int tid = omp_get_thread_num();
  int nthreads = omp_get_num_threads();
  printf("Hello from thread %d out of %d threads\n", tid, nthreads);
  printf("This is inside thread_func()!\n");
}

int main(int argc, char** argv) {

#pragma omp parallel num_threads(4) 
  {
    thread_func();
  }

  return 0;
}
