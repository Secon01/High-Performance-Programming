#include <stdio.h>
#include <pthread.h>

/* Shared state: worker thread must wait until main sets this to 1. */
int DoItNow = 0;

/* Mutex protects DoItNow; condition variable lets the worker sleep efficiently. */
pthread_mutex_t m;
pthread_cond_t c;

void* thread_func(void* arg) {
  (void)arg;

  printf("This is thread_func() starting, now entering loop to wait until DoItNow is set...\n");

  /* Condition variables must be used with a mutex guarding the predicate (DoItNow). */
  pthread_mutex_lock(&m);

  /* Use a while-loop (not if) to handle spurious wakeups safely. */
  while (DoItNow == 0) {
    /* Atomically: release m and sleep; when awakened, re-acquire m before returning. */
    pthread_cond_wait(&c, &m);
  }

  pthread_mutex_unlock(&m);

  printf("This is thread_func() after the loop.\n");
  return NULL;
}

int main() {
  printf("This is the main() function starting.\n");

  pthread_mutex_init(&m, NULL);
  pthread_cond_init(&c, NULL);

  /* Start worker thread (it will block in pthread_cond_wait). */
  pthread_t thread;
  printf("the main() function now calling pthread_create().\n");
  pthread_create(&thread, NULL, thread_func, NULL);
  printf("This is the main() function after pthread_create()\n");

  /* Simulated work in main; worker should not spin and waste CPU during this time. */
  long int k;
  double x = 1;
  for(k = 0; k < 2000000000; k++)
    x *= 1.00000000001;
  printf("main thread did some work, x = %f\n", x);

  /* Update shared state under the mutex, then signal the waiting thread. */
  pthread_mutex_lock(&m);
  DoItNow = 1;
  //pthread_cond_signal(&c);
  pthread_mutex_unlock(&m);

  /* Join ensures the worker has observed DoItNow and finished before exit. */
  printf("the main() function now calling pthread_join().\n");
  pthread_join(thread, NULL);
  printf("This is the main() function after calling pthread_join().\n");

  pthread_cond_destroy(&c);
  pthread_mutex_destroy(&m);

  return 0;
}