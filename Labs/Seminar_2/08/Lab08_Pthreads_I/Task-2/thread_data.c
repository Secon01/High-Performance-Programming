#include <stdio.h>
#include <pthread.h>

void* the_thread_func(void* arg) {
  /* Do something here? */
  double *arr = (double *) arg;
  printf("%.1f\n", arr[0]);
  printf("%.1f\n", arr[1]);
  printf("%.1f\n", arr[2]);
  return NULL;
}

int main() {
  printf("This is the main() function starting.\n");

  double data_for_thread[3];
  data_for_thread[0] = 5.7;
  data_for_thread[1] = 9.2;
  data_for_thread[2] = 1.6;

  double data_for_thread_B[3] = {5.5, 9.4, 1.4};


  /* Start thread. */
  pthread_t thread;
  printf("the main() function now calling pthread_create().\n");
  pthread_create(&thread, NULL, the_thread_func, data_for_thread);

  pthread_t threadB;
  pthread_create(&threadB, NULL, the_thread_func, data_for_thread_B);

  printf("This is the main() function after pthread_create()\n");

  /* Do something here? */

  /* Wait for thread to finish. */
  printf("the main() function now calling pthread_join().\n");
  pthread_join(thread, NULL);
  pthread_join(threadB, NULL);
  return 0;
}
