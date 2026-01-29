#include <stdio.h>
#include <stdlib.h>

// Calculate binomial coefficients
long long binomial(int n, int k)
{
    if(k<0 || k>n) return 0;    // 0 <= k <= n        
    if(k > n - k) k = n-k;      // binomial coefficients are symmetric    
    long long product = 1;
    for (int i = 1; i <= k; i++)
    {
        product = product * (n + 1 - i) / i;
    }
    return product;
}

// Fill in the lower triangular matrix
void fill_in_matrix(long long **matrix, int n)
{
    for (int i = 0; i < n; i++)
    {
        matrix[i] = malloc((size_t)(i+1) * sizeof(matrix[i]));  // allocate memory for columns
        if(!matrix[i]) exit(1);
        for (int j = 0; j <= i; j++)
        {
            matrix[i][j] = binomial(i, j);
        }
        
    }
    
}

// Printing the matrix
void print_matrix(long long **matrix, int n)
{
    for (int i = 0; i < n; i++)
    {
        printf("\n");
        for (int j = 0; j <= i; j++)
        {
            printf("%lld ", matrix[i][j] = binomial(i, j));
        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);      // get argument
    long long **matrix = malloc((size_t)n * sizeof(*matrix));
    if (!matrix) return 1;

    fill_in_matrix(matrix, n);
    print_matrix(matrix, n);
    for (int i = 0; i < n; i++) free(matrix[i]);
    free(matrix);
    return 0;
}