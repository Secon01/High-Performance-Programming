#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    long index;
    long start;
    long end;     
    long count;   
} thread_data_t;

double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

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
    if (argc != 3) {
        printf("Usage: %s M N\n", argv[0]);
        return 1;
    } 

    long M = atol(argv[1]);
    long N = atol(argv[2]);
    pthread_t *threads = malloc(N * sizeof(pthread_t)); 
    thread_data_t *data = malloc(N * sizeof(thread_data_t));
    long chunk = M / N;                                     // base size
    long rem   = M % N;                                     // extra numbers leftover 
    double t0 = get_wall_seconds();   
    if (N < 1) { printf("N must be >= 1\n"); return 1; }
    for (long t = 0; t < N; t++)
    {
        data[t].index = t;
        long extra = (t < rem) ? 1 : 0;                     // determine which threads will get the extra numbers
        data[t].start = 1 + t*chunk + (t < rem ? t : rem);  
        data[t].end = data[t].start + chunk + extra;
        data[t].count = 0;
        pthread_create(&threads[t], NULL, thread_count_primes, &data[t]);
    }

    long total = 0;
    for (long t = 0; t < N; t++) {
        pthread_join(threads[t], NULL);
        total += data[t].count;
    }
    double t1 = get_wall_seconds();
    printf("Wall time: %f seconds\n", t1 - t0);
    printf("There are %ld primes!\n", total);
    free(data);
    free(threads);
    return 0;
}