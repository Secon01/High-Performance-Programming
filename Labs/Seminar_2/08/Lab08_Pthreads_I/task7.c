#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* child_thread_func(void* arg)
{
    printf("Hello I am a child thread\n");
    return NULL;
}

void* thread_func(void* arg)
{
    printf("Hello I am a parent thread\n");
    pthread_t ct1, ct2;
    pthread_create(&ct1, NULL, child_thread_func, NULL);
    pthread_create(&ct2, NULL, child_thread_func, NULL);

    pthread_join(ct1, NULL);
    pthread_join(ct2, NULL);
    return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t pt1, pt2;
    pthread_create(&pt1, NULL, thread_func, NULL);
    pthread_create(&pt2, NULL, thread_func, NULL);

    pthread_join(pt1, NULL);
    pthread_join(pt2, NULL);
    return 0;
}
