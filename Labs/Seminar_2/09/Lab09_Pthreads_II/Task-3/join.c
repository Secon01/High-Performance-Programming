/*****************************************************************************
 * FILE: join.c
 *
 *****************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define NUM_THREADS 4

void *BusyWork(void *t)
{
  int i;
  long tid;
  double result = 0.0;

  tid = (long)t;
  printf("Thread %ld starting...\n", tid);

  for (i = 0; i < 20000000; i++) {
    result = result + sin(i) * tan(i);
  }

  printf("Thread %ld done. Result = %e\n", tid, result);
  pthread_exit((void*)t);
}

int main(int argc, char *argv[])
{
  pthread_t thread[NUM_THREADS];
  pthread_attr_t attr;
  int rc;
  long t;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  for (t = 0; t < NUM_THREADS; t++) {
    printf("Main: creating thread %ld\n", t);
    rc = pthread_create(&thread[t], &attr, BusyWork, (void *)t);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  pthread_attr_destroy(&attr);

  printf("Main: program completed. Exiting.\n");
  return 0;
}