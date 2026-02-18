/**********************************************************************
 * Enumeration sort
 *
 **********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define NUM_THREADS 5
#define len 100000

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

double indata[len], outdata[len];

static inline int less_with_tie(double a_i, long i, double a_j, long j)
{
  /* Tie-break guarantees unique ranks when values are equal. */
  return (a_i < a_j) || (a_i == a_j && i < j);
}

void *findrank(void *arg)
{
  int rank;
  long i;
  long j = (long)arg;

  rank = 0;
  for (i = 0; i < len; i++)
    if (less_with_tie(indata[i], i, indata[j], j)) rank++;

  outdata[rank] = indata[j];
  pthread_exit(NULL);
}

typedef struct {
  long start;
  long end;
} chunk_arg_t;

void *findranks_chunk(void *arg)
{
  chunk_arg_t *a = (chunk_arg_t *)arg;

  /* One thread computes ranks for many elements to amortize creation overhead. */
  for (long j = a->start; j < a->end; j++) {
    int rank = 0;
    for (long i = 0; i < len; i++)
      if (less_with_tie(indata[i], i, indata[j], j)) rank++;
    outdata[rank] = indata[j];
  }

  return NULL;
}

static void reset_outdata(void)
{
  for (int i = 0; i < len; i++) outdata[i] = -1.0;
}

static void check_sorted(void)
{
  for (int i = 0; i < len - 1; i++)
    if (outdata[i] > outdata[i + 1])
      printf("ERROR: %f,%f\n", outdata[i], outdata[i + 1]);
}

int main(int argc, char *argv[]) {

  pthread_t threads[NUM_THREADS];
  pthread_attr_t attr;
  int i, j, t;
  long el;
  void *status;

  int mode = 0; /* 0=both, 1=approach1, 2=approach2 */
  if (argc > 1) mode = atoi(argv[1]);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  /* Generate random numbers once (same input for both approaches). */
  for (i = 0; i < len; i++) indata[i] = drand48();

  if (mode == 0 || mode == 1) {
    reset_outdata();

    /* Approach 1: create/join threads repeatedly (high overhead). */
    double startTime = get_wall_seconds();
    for (j = 0; j < len; j += NUM_THREADS) {
      for (t = 0; t < NUM_THREADS; t++) {
        el = j + t;
        if (el < len)
          pthread_create(&threads[t], &attr, findrank, (void *)el);
      }
      for (t = 0; t < NUM_THREADS; t++) {
        el = j + t;
        if (el < len)
          pthread_join(threads[t], &status);
      }
    }
    double timeTaken = get_wall_seconds() - startTime;
    printf("Approach1 (many create/join): Time: %f  NUM_THREADS: %d\n",
           timeTaken, NUM_THREADS);

    check_sorted();
  }

  if (mode == 0 || mode == 2) {
    reset_outdata();

    /* Approach 2: create NUM_THREADS once; each thread ranks a block of elements. */
    chunk_arg_t args[NUM_THREADS];
    long base = len / NUM_THREADS;
    long rem  = len % NUM_THREADS;

    long next = 0;
    double startTime = get_wall_seconds();
    for (t = 0; t < NUM_THREADS; t++) {
      long chunk = base + (t < rem ? 1 : 0);
      args[t].start = next;
      args[t].end   = next + chunk;
      next = args[t].end;
      pthread_create(&threads[t], &attr, findranks_chunk, &args[t]);
    }
    for (t = 0; t < NUM_THREADS; t++)
      pthread_join(threads[t], &status);

    double timeTaken = get_wall_seconds() - startTime;
    printf("Approach2 (chunk per thread): Time: %f  NUM_THREADS: %d\n",
           timeTaken, NUM_THREADS);

    check_sorted();
  }

  return 0;
}