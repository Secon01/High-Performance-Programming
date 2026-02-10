#include <stdio.h>
#include <pthread.h>
#include <unistd.h>   // sleep()

void* the_thread_func(void* arg) {
  /* Do something here? */
  printf("thread_func() starting doing some work.\n");
  long int i;
  volatile double sum = 0.0;
  for(i = 0; i < 20000000000L; i++)
  sum += 1e-7;
  printf("Result of work in thread_func(): sum = %f\n", sum);
  return NULL;
}

void* the_thread_func_Β(void* arg) {
  /* Do something here? */
  printf("thread_func() starting doing some work.\n");
  long int i;
  volatile double sum = 0.0;
  for(i = 0; i < 20000000000L; i++)
  sum += 1e-7;
  printf("Result of work in thread_func(): sum = %f\n", sum);
  return NULL;
}

int main() {
  printf("This is the main() function starting.\n");

  /* Start thread. */
  pthread_t thread;
  printf("the main() function now calling pthread_create().\n");
  pthread_create(&thread, NULL, the_thread_func, NULL);

  pthread_t threadΒ;
  pthread_create(&threadΒ, NULL, the_thread_func_Β, NULL);
  printf("This is the main() function after pthread_create()\n");

  /* Do something here? */
  printf("main() starting doing some work.\n");
  long int i;
  volatile double sum = 0.0;
  for(i = 0; i < 20000000000L; i++)
  sum += 1e-7;
  printf("Result of work in main(): sum = %f\n", sum);
  printf("the main() function now calling pthread_join().\n");
  /* Wait for thread to finish. */
  pthread_join(thread, NULL);
  pthread_join(threadΒ, NULL);
  return 0;
}
