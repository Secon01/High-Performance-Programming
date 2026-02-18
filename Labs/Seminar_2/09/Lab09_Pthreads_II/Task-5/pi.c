/**********************************************************************
 * This program calculates pi using C
 *
 **********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
  long start;
  long end;
  double dx;
  double *out;   /* per-thread output slot: avoids mutex on a global sum */
} thread_arg_t;

static void *worker(void *arg)
{
  thread_arg_t *a = (thread_arg_t *)arg;
  double sum = 0.0;

  /* Each thread integrates its own contiguous sub-interval range. */
  for (long i = a->start; i <= a->end; i++) {
    double x = a->dx * ((double)i - 0.5);
    sum += a->dx * 4.0 / (1.0 + x * x);
  }

  *(a->out) = sum;
  return NULL;
}

int main(int argc, char *argv[])
{
  const long intervals_default = 500000000;
  int num_threads = 4;
  long intervals = intervals_default;

  if (argc > 1) num_threads = atoi(argv[1]);
  if (argc > 2) intervals = atol(argv[2]);
  if (num_threads < 1) num_threads = 1;
  if (intervals < 1) intervals = 1;

  pthread_t *threads = (pthread_t *)malloc((size_t)num_threads * sizeof(*threads));
  thread_arg_t *args = (thread_arg_t *)malloc((size_t)num_threads * sizeof(*args));
  double *partial = (double *)malloc((size_t)num_threads * sizeof(*partial));

  if (!threads || !args || !partial) {
    fprintf(stderr, "Allocation failed\n");
    free(threads); free(args); free(partial);
    return 1;
  }

  double dx = 1.0 / (double)intervals;

  /* Static block distribution; remainder handled by giving the first 'rem' threads one extra interval. */
  long base = intervals / num_threads;
  long rem  = intervals % num_threads;

  long next = 1;
  for (int t = 0; t < num_threads; t++) {
    long chunk = base + (t < rem ? 1 : 0);

    args[t].start = next;
    args[t].end   = next + chunk - 1;
    args[t].dx    = dx;
    args[t].out   = &partial[t];
    partial[t] = 0.0;

    pthread_create(&threads[t], NULL, worker, &args[t]);
    next = args[t].end + 1;
  }

  /* Join is the synchronization point: guarantees all partial sums are ready before reduction. */
  for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
  }

  /* Deterministic reduction order here; still differs from a purely serial loop due to grouping of additions. */
  double sum = 0.0;
  for (int t = 0; t < num_threads; t++) sum += partial[t];

  printf("PI is approx. %.16f\n", sum);

  free(threads);
  free(args);
  free(partial);

  return 0;
}