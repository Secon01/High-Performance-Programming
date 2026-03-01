#include <stdlib.h>
#include "sort_funcs.h"
#include <omp.h>

void bubble_sort(intType* list, int N) {
  int i, j;
  for(i = 0; i < N-1; i++)
    for(j = 1+i; j < N; j++) {
      if(list[i] > list[j]) {
	// Swap
	intType tmp = list[i];
	list[i] = list[j];
	list[j] = tmp;
      }
    }
}

void merge_sort(intType* list_to_sort, int N, int nThreads) {
  if(N <= 1) return;

  int n1 = N / 2;
  int n2 = N - n1;

  // One malloc instead of two (reduces malloc/free contention when threaded)
  intType* tmp = (intType*)malloc(N * sizeof(intType));
  intType* list1 = tmp;
  intType* list2 = tmp + n1;

  int i;
  for(i = 0; i < n1; i++)
    list1[i] = list_to_sort[i];
  for(i = 0; i < n2; i++)
    list2[i] = list_to_sort[n1+i];

  // Nested parallel recursion: split work into 2 threads when allowed
  if(nThreads >= 2) {
    #pragma omp parallel num_threads(2)
    {
      int tid = omp_get_thread_num();
      if(tid == 0) merge_sort(list1, n1, nThreads/2);
      else         merge_sort(list2, n2, nThreads/2);
    }
  } else {
    merge_sort(list1, n1, 1);
    merge_sort(list2, n2, 1);
  }

  // Merge back into list_to_sort
  int i1 = 0, i2 = 0;
  i = 0;
  while(i1 < n1 && i2 < n2) {
    if(list1[i1] < list2[i2]) list_to_sort[i++] = list1[i1++];
    else                      list_to_sort[i++] = list2[i2++];
  }
  while(i1 < n1) list_to_sort[i++] = list1[i1++];
  while(i2 < n2) list_to_sort[i++] = list2[i2++];

  free(tmp);
}