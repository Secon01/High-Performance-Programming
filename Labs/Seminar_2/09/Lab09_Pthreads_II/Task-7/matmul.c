#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

double **A,**B,**C;
int n;

typedef struct {
  int row_start;
  int row_end;
} thread_arg_t;

static void *matmul_rows(void *arg)
{
  thread_arg_t *a = (thread_arg_t *)arg;

  /* Thread-private rows of C -> no locks required. */
  for (int i = a->row_start; i < a->row_end; i++)
    for (int j = 0; j < n; j++) {
      double sum = 0.0;
      for (int k = 0; k < n; k++)
        sum += A[i][k] * B[k][j];
      C[i][j] = sum;
    }

  return NULL;
}

int main(int argc, char *argv[]) {
  int i, j, k;

  if(argc != 2 && argc != 3) {
    printf("Usage: %s <matrix_size_n> [num_threads]\n", argv[0]);
    return -1;
  }

  n = atoi(argv[1]);
  int num_threads = (argc == 3) ? atoi(argv[2]) : 4;
  if (num_threads < 1) num_threads = 1;
  if (num_threads > n) num_threads = n;

  //Allocate and fill matrices
  A = (double **)malloc(n*sizeof(double *));
  B = (double **)malloc(n*sizeof(double *));
  C = (double **)malloc(n*sizeof(double *));
  for(i=0;i<n;i++){
    A[i] = (double *)malloc(n*sizeof(double));
    B[i] = (double *)malloc(n*sizeof(double));
    C[i] = (double *)malloc(n*sizeof(double));
  }

  for (i = 0; i<n; i++)
    for(j=0;j<n;j++){
      A[i][j] = rand() % 5 + 1;
      B[i][j] = rand() % 5 + 1;
      C[i][j] = 0.0;
    }

  printf("Doing matrix-matrix multiplication...\n");
  double startTime = get_wall_seconds();

  pthread_t *threads = (pthread_t *)malloc((size_t)num_threads * sizeof(*threads));
  thread_arg_t *args  = (thread_arg_t *)malloc((size_t)num_threads * sizeof(*args));
  if (!threads || !args) {
    printf("Allocation failed\n");
    free(threads);
    free(args);
    return -1;
  }

  /* Static row-block distribution to balance work and avoid synchronization. */
  int base = n / num_threads;
  int rem  = n % num_threads;
  int next = 0;

  for (int t = 0; t < num_threads; t++) {
    int rows = base + (t < rem ? 1 : 0);
    args[t].row_start = next;
    args[t].row_end   = next + rows;
    next = args[t].row_end;
    pthread_create(&threads[t], NULL, matmul_rows, &args[t]);
  }

  /* Join is the only required synchronization before correctness check. */
  for (int t = 0; t < num_threads; t++)
    pthread_join(threads[t], NULL);

  free(threads);
  free(args);

  double timeTaken = get_wall_seconds() - startTime;
  printf("Elapsed time: %f wall seconds\n", timeTaken);

  // Correctness check (let this part remain serial)
  printf("Verifying result correctness for a few result matrix elements...\n");
  int nElementsToVerify = 5*n;
  double max_abs_diff = 0;
  for(int el = 0; el < nElementsToVerify; el++) {
    i = rand() % n;
    j = rand() % n;
    double Cij = 0;
    for(k = 0; k < n; k++)
      Cij += A[i][k] * B[k][j];
    double abs_diff = fabs(C[i][j] - Cij);
    if(abs_diff > max_abs_diff)
      max_abs_diff = abs_diff;
  }
  printf("max_abs_diff = %g\n", max_abs_diff);
  if(max_abs_diff > 1e-10) {
    for(i = 0; i < 10; i++)
      printf("ERROR: TOO LARGE DIFF. SOMETHING IS WRONG.\n");
    return -1;
  }
  printf("OK -- result seems correct!\n");
  
  // Free memory
  for(i=0;i<n;i++){
    free(A[i]);
    free(B[i]);
    free(C[i]);
  }
  free(A);
  free(B);
  free(C);

  return 0;
}