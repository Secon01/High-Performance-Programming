#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    long start;
    long end;     
    long count;   
} thread_data_t;

int is_prime(long n)
{
    if(n<2) return 0;
    for (long i = 2; i < n; i++)
    {
        if (n % i == 0)
        {
            return 0;
        }
    }
    return 1;
}

void* thread_count_primes(void* arg)
{
    thread_data_t* data = (thread_data_t*)arg;
    long primes = 0;
    for (long i = data->start; i < data->end; i++)
    {
        if (is_prime(i))
        {
            primes++;
        }
    }
    data->count = primes;
    return NULL;
}

int main(int argc, char const *argv[])
{
    long M = atol(argv[1]);
    if (argc != 2) {
        printf("Usage: %s M\n", argv[0]);
        return 1;
    } 

    pthread_t t1, t2;
    thread_data_t A = {.start = 1, .end = M/2, .count = 0};
    thread_data_t B = {.start = M/2 + 1, .end = M, .count = 0};
    pthread_create(&t1, NULL, thread_count_primes, &A);
    pthread_create(&t2, NULL, thread_count_primes, &B);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    long primes = A.count + B.count;
    printf("There are %ld primes!\n", primes);
    return 0;
}