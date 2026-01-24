#include <stdio.h>
#include <stdlib.h>

int is_prime(int n)
{
    if (n <= 1)
        return 0;   // not prime

    for (int i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
            return 0;  // divisor found
    }

    return 1;  // prime
}
int main(void)
{
    int n;
    scanf("%d", &n);
    int *arr;
    arr = (int *)malloc(n*sizeof(*arr));
    if (!arr) return 1;                         /* check if malloc failed */
    for(int i=0; i<n; i++) 
    {
        arr[i] = rand()%100;                    /* random number from 0 to 99 */
    }
    int *non_primes = NULL;
    int count = 0;
    for (int i=0; i<n; i++)
    {
        if(!is_prime(arr[i]))
        {
            int *tmp = realloc(non_primes, (count + 1) * sizeof(*tmp));
            if(!tmp)
            {
                free(non_primes);
                free(arr);
                return 1;
            }
            non_primes = tmp;
            non_primes[count] = arr[i];
            count++;
        }
    }
    printf("\nSize of new array: %d", count);
    for (int i = 0; i < count; i++)
    {
        printf("\n%d", non_primes[i]);
    }
    free(arr);
    free(non_primes);
    return 0;
}