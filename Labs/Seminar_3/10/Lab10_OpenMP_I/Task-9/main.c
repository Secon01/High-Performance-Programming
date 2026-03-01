#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include "sort_funcs.h"

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

static int count_values(const intType* list, int n, intType x) {
  int count = 0;
  for(int i = 0; i < n; i++) {
    if(list[i] == x) count++;
  }
  return count;
}

int main(int argc, char* argv[]) {
  if(argc != 3) {
    printf("Please give 2 arguments: N (number of elements) and nThreads.\n");
    return -1;
  }

  int N = atoi(argv[1]);
  int nThreads = atoi(argv[2]);

  if(N < 1) {
    printf("Error: (N < 1).\n");
    return -1;
  }
  if(nThreads < 1) {
    printf("Error: (nThreads < 1).\n");
    return -1;
  }

  printf("N = %d\n", N);
  printf("nThreads = %d\n", nThreads);

  // Nested parallelism demo
  printf("omp_get_nested() initially = %d\n", omp_get_nested());

  omp_set_nested(0);
  printf("After omp_set_nested(0): omp_get_nested() = %d\n", omp_get_nested());
  #pragma omp parallel num_threads(2)
  {
    printf("[outer] tid=%d / %d\n", omp_get_thread_num(), omp_get_num_threads());
    #pragma omp parallel num_threads(2)
    {
      printf("  [inner] tid=%d / %d\n", omp_get_thread_num(), omp_get_num_threads());
    }
  }

  omp_set_nested(1);
  printf("After omp_set_nested(1): omp_get_nested() = %d\n", omp_get_nested());
  #pragma omp parallel num_threads(2)
  {
    printf("[outer] tid=%d / %d\n", omp_get_thread_num(), omp_get_num_threads());
    #pragma omp parallel num_threads(2)
    {
      printf("  [inner] tid=%d / %d\n", omp_get_thread_num(), omp_get_num_threads());
    }
  }

  intType* list_to_sort = (intType*)malloc((size_t)N * sizeof(intType));
  if(!list_to_sort) {
    printf("Error: malloc failed.\n");
    return -1;
  }

  // Fill list with random numbers
  for(int i = 0; i < N; i++)
    list_to_sort[i] = rand() % 100;

  // Count how many times the number 7 exists in the list.
  int count7 = count_values(list_to_sort, N, 7);
  printf("Before sort: the number 7 occurs %d times in the list.\n", count7);

  // Sort list (enable nested for the recursive parallel regions)
  omp_set_nested(1);
  double time1 = get_wall_seconds();
  merge_sort(list_to_sort, N, nThreads);
  printf("Sorting list with length %d took %7.3f wall seconds.\n",
         N, get_wall_seconds() - time1);

  int count7_again = count_values(list_to_sort, N, 7);
  printf("After sort : the number 7 occurs %d times in the list.\n", count7_again);

  // Check that list is really sorted
  for(int i = 0; i < N - 1; i++) {
    if(list_to_sort[i] > list_to_sort[i + 1]) {
      printf("Error! List not sorted!\n");
      free(list_to_sort);
      return -1;
    }
  }
  printf("OK, list is sorted!\n");

  free(list_to_sort);
  return 0;
}