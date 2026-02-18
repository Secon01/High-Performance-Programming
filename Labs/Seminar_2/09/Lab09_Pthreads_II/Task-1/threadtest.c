#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
  
void* the_thread_func(void* arg) {
  (void)arg;

  /* allocate memory dynamically in the thread */
  int *p = (int*)malloc(3 * sizeof(int));
  if (p == NULL) {
      /* returning NULL signals allocation failure */
      return NULL;
  }

  /* store some values */
  p[0] = 11;
  p[1] = 22;
  p[2] = 33;

    /* return pointer to main via pthread_join */
    return (void*)p;
  return NULL;
}

int main() {
  printf("This is the main() function starting.\n");

  /* Start thread. */
  pthread_t thread;
  printf("the main() function now calling pthread_create().\n");
  if(pthread_create(&thread, NULL, the_thread_func, NULL) != 0) {
    printf("ERROR: pthread_create failed.\n");
    return -1;
  }

  printf("This is the main() function after pthread_create()\n");

  /* Wait for thread to finish. */
  printf("the main() function now calling pthread_join().\n");
  void *ret = NULL;
  if(pthread_join(thread, &ret) != 0) {
    printf("ERROR: pthread_join failed.\n");
    return -1;
  }
  int *p = (int*)ret;
  if (p == NULL) {
    printf("ERROR: thread returned NULL.\n");
    return -1;
  }
  printf("Values from thread: %d %d %d\n", p[0], p[1], p[2]);
  free(p);

  return 0;
}