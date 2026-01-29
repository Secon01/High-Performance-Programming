#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

double** allocate_matrix(int n);
void deallocate_matrix(double** theMatrix, int n);
void fill_matrix(double** theMatrix, int n);
void print_matrix(double** theMatrix, int n);
double get_min_value(double** theMatrix, int n);
double get_max_value(double** theMatrix, int n);

/* helper timer */
double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

int main()
{
    int n;
    printf("\nEnter the dimension for a square matrix:");
    scanf("%d",&n);
    printf("n = %d\n", n);

    double t0, t1;

    // --- allocate A ---
    t0 = get_wall_seconds();
    double** matrixA = allocate_matrix(n);
    t1 = get_wall_seconds();
    printf("allocate_matrix(A): %.6f s\n", t1 - t0);

    // --- fill A ---
    t0 = get_wall_seconds();
    fill_matrix(matrixA, n);
    t1 = get_wall_seconds();
    printf("fill_matrix(A):     %.6f s\n", t1 - t0);

    // --- min ---
    t0 = get_wall_seconds();
    double minValue = get_min_value(matrixA, n);
    t1 = get_wall_seconds();
    printf("get_min_value(A):   %.6f s\n", t1 - t0);

    // --- max ---
    t0 = get_wall_seconds();
    double maxValue = get_max_value(matrixA, n);
    t1 = get_wall_seconds();
    printf("get_max_value(A):   %.6f s\n", t1 - t0);

    printf("Min value: %14.9f  Max value: %14.9f\n", minValue, maxValue);

    // --- free A ---
    t0 = get_wall_seconds();
    deallocate_matrix(matrixA, n);
    t1 = get_wall_seconds();
    printf("deallocate_matrix(A): %.6f s\n", t1 - t0);

    // --- Question 2 extra allocation/free (time it too) ---
    t0 = get_wall_seconds();
    double** matrixB = allocate_matrix(n);
    t1 = get_wall_seconds();
    printf("allocate_matrix(B): %.6f s\n", t1 - t0);

    t0 = get_wall_seconds();
    deallocate_matrix(matrixB,n);
    t1 = get_wall_seconds();
    printf("deallocate_matrix(B): %.6f s\n", t1 - t0);

    return 0;
}

double** allocate_matrix(int n)
{
  double** theMatrix;
  int i;
  theMatrix = calloc(n , sizeof(double *));
  for(i = 0; i < n; i++)
    theMatrix[i] = calloc(n , sizeof(double));
  return theMatrix;
}

void deallocate_matrix(double** theMatrix, int n)
{
  int i;
  for(i = 0; i < n; i++)
    free(theMatrix[i]);
  free(theMatrix);
}

void fill_matrix(double** theMatrix, int n)
{
  int j, i;
  for(j = 0; j < n; j++)
    for(i = 0 ; i < n ; i++)
      theMatrix[i][j] = 10 * (double)rand() / RAND_MAX;
}

void print_matrix(double** theMatrix, int n)
{
  int i, j;
  for(i = 0 ; i < n; i++)
    {
      for(j = 0 ; j < n ; j++)
	printf("%2.3f " , theMatrix[i][j]);
      putchar('\n');
    }
}

double get_max_value(double** theMatrix, int n)
{
  int i, j;
  double max = 0.0;
  max = theMatrix[0][0];
  for(j = 0; j < n ; j++)
    for(i = 0 ;i < n; i++)
      if(max < theMatrix[i][j])
	max = theMatrix[i][j];
  return max;
}

double get_min_value(double** theMatrix, int n)
{
  int i, j;
  double min = 0.0;
  min = theMatrix[0][0];
  for(j = 0; j < n; j++)
    for(i = 0; i < n; i++)
      if(min > theMatrix[i][j])
	min = theMatrix[i][j];
  return min;
}
