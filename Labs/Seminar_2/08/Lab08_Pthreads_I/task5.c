#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>   // sleep()

typedef struct {
    long index;
} thread_data_t;

void* thread_func(void* arg)
{
    thread_data_t *data = (thread_data_t*)arg;
    printf("Thread %ld\n", data->index);
    return NULL;
}

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        printf("Usage: %s M\n", argv[0]);
        return 1;
    }

    long N = atol(argv[1]);
    pthread_t *threads = malloc(N * sizeof(pthread_t)); 
    thread_data_t *data = malloc(N * sizeof(thread_data_t));
    for (long i = 0; i < N; i++)
    {
        data[i].index = i;
        int rc = pthread_create(&threads[i], NULL, thread_func, &data[i]);
        if (rc != 0) {
            printf("pthread_create failed at i=%ld (rc=%d)\n", i, rc);
            break;
        }
    }
    for (long i = 0; i < N; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(data);
    free(threads);
    return 0;
}