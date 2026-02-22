#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t  cv;
    int nthreads;   // total threads
    int waiting;    // how many threads have arrived
    int phase;      // toggles each barrier use (0/1 or incrementing)
} barrier_t;

// Barrier initialization
static inline int barrier_init(barrier_t *b, int nthreads) {
    b->nthreads = nthreads;     // store number of participating threads
    b->waiting  = 0;            // no thread has arrived yet
    b->phase    = 0;            // start at phase 0
    if (pthread_mutex_init(&b->lock, NULL) != 0) return -1; // initialize mutex
    if (pthread_cond_init(&b->cv, NULL) != 0) return -1;    // initialize condition variable
    return 0;
}
// Free barrier recourses
static inline void barrier_destroy(barrier_t *b) {
    pthread_cond_destroy(&b->cv);       // destroy condition variable
    pthread_mutex_destroy(&b->lock);    // destroy mutex
}
// Wait untill all threads arrive
static inline void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->lock);       // enter critical section

    int my_phase = b->phase;            // save current phase (generation)
    b->waiting++;                       // increase the number of waiting threads

    if (b->waiting == b->nthreads) {    // last thread arrives
        b->waiting = 0;                 // reset counter for next use
        b->phase = 1 - b->phase;        // flip phase to release current waiting threads
        pthread_cond_broadcast(&b->cv); // wake up all waiting threads
    } else {
        // wait until last thread flips phase
        while (b->phase == my_phase) {
            pthread_cond_wait(&b->cv, &b->lock);
        }
    }

    pthread_mutex_unlock(&b->lock);     // leave critical section
}
#endif