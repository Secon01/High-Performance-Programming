#include "gol.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/*
 * Shared state for the persistent-worker version.
 * The main thread publishes one generation at a time, workers compute their
 * own row blocks, and then the main thread waits until all workers finish.
 */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t step_ready_cv;
    pthread_cond_t step_done_cv;

    const uint8_t *grid;   /* Read-only buffer for the current generation. */
    uint8_t *next;         /* Output buffer for the next generation. */

    int worker_count;      /* Number of background workers (main thread excluded). */
    int published_step;    /* Monotonic generation counter published by the main thread. */
    int completed_workers; /* Workers that have finished the current generation. */
    int stop;              /* Set to 1 when workers should exit. */
} sync_state_t;

typedef struct {
    int start_row;
    int end_row;
    int width;
    sync_state_t *sync;
} worker_arg_t;

static void usage(const char *prog)
{
    printf("Usage: %s [-W width] [-H height] [-s steps] [-t threads] [-p prob] [-seed N] [-print]\n",
           prog);
}

static double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

/*
 * Split interior rows [1, height - 1) into almost equal contiguous blocks.
 * Contiguous blocks are simple, cache-friendly, and easy to explain.
 */
static void get_row_range(int tid, int total_threads, int height,
                          int *start_row, int *end_row)
{
    const int first = 1;
    const int last = (height > 1) ? (height - 1) : 1;
    const int interior_rows = last - first;

    if (interior_rows <= 0) {
        *start_row = first;
        *end_row = first;
        return;
    }

    const int base = interior_rows / total_threads;
    const int rem = interior_rows % total_threads;
    const int offset = tid * base + (tid < rem ? tid : rem);
    const int count = base + (tid < rem ? 1 : 0);

    *start_row = first + offset;
    *end_row = *start_row + count;
}

static int sync_init(sync_state_t *sync, int worker_count)
{
    sync->grid = NULL;
    sync->next = NULL;
    sync->worker_count = worker_count;
    sync->published_step = 0;
    sync->completed_workers = 0;
    sync->stop = 0;

    if (pthread_mutex_init(&sync->mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&sync->step_ready_cv, NULL) != 0) {
        pthread_mutex_destroy(&sync->mutex);
        return -1;
    }
    if (pthread_cond_init(&sync->step_done_cv, NULL) != 0) {
        pthread_cond_destroy(&sync->step_ready_cv);
        pthread_mutex_destroy(&sync->mutex);
        return -1;
    }

    return 0;
}

static void sync_destroy(sync_state_t *sync)
{
    pthread_cond_destroy(&sync->step_done_cv);
    pthread_cond_destroy(&sync->step_ready_cv);
    pthread_mutex_destroy(&sync->mutex);
}

/* Publish the buffers for one new generation and wake all workers. */
static void sync_start_step(sync_state_t *sync, const uint8_t *grid, uint8_t *next)
{
    pthread_mutex_lock(&sync->mutex);
    sync->grid = grid;
    sync->next = next;
    sync->completed_workers = 0;
    sync->published_step++;
    pthread_cond_broadcast(&sync->step_ready_cv);
    pthread_mutex_unlock(&sync->mutex);
}

/* Wait until every background worker has finished the current generation. */
static void sync_wait_step_done(sync_state_t *sync)
{
    pthread_mutex_lock(&sync->mutex);
    while (sync->completed_workers < sync->worker_count) {
        pthread_cond_wait(&sync->step_done_cv, &sync->mutex);
    }
    pthread_mutex_unlock(&sync->mutex);
}

static void sync_stop_workers(sync_state_t *sync)
{
    pthread_mutex_lock(&sync->mutex);
    sync->stop = 1;
    pthread_cond_broadcast(&sync->step_ready_cv);
    pthread_mutex_unlock(&sync->mutex);
}

static void *worker_main(void *arg)
{
    worker_arg_t *a = (worker_arg_t *)arg;
    sync_state_t *sync = a->sync;
    int seen_step = 0;

    pthread_mutex_lock(&sync->mutex);
    for (;;) {
        /* Sleep until the main thread publishes a new generation. */
        while (!sync->stop && seen_step == sync->published_step) {
            pthread_cond_wait(&sync->step_ready_cv, &sync->mutex);
        }

        if (sync->stop) {
            pthread_mutex_unlock(&sync->mutex);
            return NULL;
        }

        /* Copy the shared pointers while holding the mutex, then compute outside it. */
        const uint8_t *grid = sync->grid;
        uint8_t *next = sync->next;
        const int current_step = sync->published_step;
        pthread_mutex_unlock(&sync->mutex);

        if (a->start_row < a->end_row) {
            step_range(grid, next, a->width, a->start_row, a->end_row);
        }

        pthread_mutex_lock(&sync->mutex);
        seen_step = current_step;
        sync->completed_workers++;
        if (sync->completed_workers == sync->worker_count) {
            pthread_cond_signal(&sync->step_done_cv);
        }
    }
}

int main(int argc, char **argv)
{
    int width = 80;
    int height = 40;
    int steps = 1000;
    int requested_threads = 1;
    double alive_probability = 0.30;
    unsigned seed = 1;
    int do_print = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-W") && i + 1 < argc) width = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-H") && i + 1 < argc) height = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) steps = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) requested_threads = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-p") && i + 1 < argc) alive_probability = atof(argv[++i]);
        else if (!strcmp(argv[i], "-seed") && i + 1 < argc) seed = (unsigned)atoi(argv[++i]);
        else if (!strcmp(argv[i], "-print")) do_print = 1;
        else {
            usage(argv[0]);
            return 1;
        }
    }

    if (width <= 0 || height <= 0 || steps < 0 || requested_threads <= 0 ||
        alive_probability < 0.0 || alive_probability > 1.0) {
        fprintf(stderr, "Invalid arguments.\n");
        return 1;
    }

    uint8_t *grid = (uint8_t *)malloc((size_t)width * (size_t)height * sizeof(uint8_t));
    uint8_t *next = (uint8_t *)malloc((size_t)width * (size_t)height * sizeof(uint8_t));
    if (grid == NULL || next == NULL) {
        perror("malloc");
        free(grid);
        free(next);
        return 1;
    }

    srand(seed);
    initialize_grid(alive_probability, grid, width, height);
    fill_zero(next, width, height);

    if (do_print) {
        print_grid(grid, width, height);
        printf("-----Simulation-----\n");
    }

    /*
     * Only interior rows [1, height - 1) are updated.
     * There is no point in creating more worker partitions than interior rows.
     */
    const int interior_rows = (height > 2) ? (height - 2) : 0;
    int total_threads = requested_threads;
    if (interior_rows > 0 && total_threads > interior_rows) {
        total_threads = interior_rows;
    }
    if (total_threads < 1) {
        total_threads = 1;
    }

    if (total_threads == 1) {
        const double t0 = get_wall_seconds();

        for (int step_id = 0; step_id < steps; step_id++) {
            step(grid, next, width, height);

            /* Double buffering avoids in-place update hazards. */
            uint8_t *tmp = grid;
            grid = next;
            next = tmp;

            if (do_print) {
                print_grid(grid, width, height);
                printf("----------------------\n");
            }
        }

        const double elapsed = get_wall_seconds() - t0;
        const double updates = (double)((width > 2) ? (width - 2) : 0) *
                               (double)((height > 2) ? (height - 2) : 0) *
                               (double)steps;

        /* Stats go to stderr so stdout can be used for grid comparison. */
        fprintf(stderr, "Threads: %d\n", total_threads);
        fprintf(stderr, "Elapsed time: %.6f seconds\n", elapsed);
        fprintf(stderr, "Updates/sec: %.3e\n",
                (elapsed > 0.0) ? (updates / elapsed) : 0.0);

        free(grid);
        free(next);
        return 0;
    }

    const int worker_count = total_threads - 1;
    pthread_t *threads = (pthread_t *)malloc((size_t)worker_count * sizeof(pthread_t));
    worker_arg_t *args = (worker_arg_t *)malloc((size_t)worker_count * sizeof(worker_arg_t));
    sync_state_t sync;
    int main_start = 1;
    int main_end = 1;

    if (threads == NULL || args == NULL) {
        perror("malloc");
        free(threads);
        free(args);
        free(grid);
        free(next);
        return 1;
    }

    if (sync_init(&sync, worker_count) != 0) {
        fprintf(stderr, "Failed to initialize thread synchronization.\n");
        free(threads);
        free(args);
        free(grid);
        free(next);
        return 1;
    }

    /* The main thread also computes one block instead of staying idle. */
    get_row_range(0, total_threads, height, &main_start, &main_end);
    for (int tid = 1; tid < total_threads; tid++) {
        get_row_range(tid, total_threads, height,
                      &args[tid - 1].start_row, &args[tid - 1].end_row);
        args[tid - 1].width = width;
        args[tid - 1].sync = &sync;

        if (pthread_create(&threads[tid - 1], NULL, worker_main, &args[tid - 1]) != 0) {
            fprintf(stderr, "pthread_create failed for worker %d\n", tid);
            sync_stop_workers(&sync);
            for (int j = 1; j < tid; j++) {
                pthread_join(threads[j - 1], NULL);
            }
            sync_destroy(&sync);
            free(threads);
            free(args);
            free(grid);
            free(next);
            return 1;
        }
    }

    const double t0 = get_wall_seconds();

    for (int step_id = 0; step_id < steps; step_id++) {
        if (width <= 2 || height <= 2) {
            fill_zero(next, width, height);
        } else {
            /* Small global tasks stay on the main thread. */
            set_borders_dead(next, width, height);
            sync_start_step(&sync, grid, next);

            if (main_start < main_end) {
                step_range(grid, next, width, main_start, main_end);
            }

            sync_wait_step_done(&sync);
        }

        uint8_t *tmp = grid;
        grid = next;
        next = tmp;

        if (do_print) {
            print_grid(grid, width, height);
            printf("----------------------\n");
        }
    }

    sync_stop_workers(&sync);
    for (int i = 0; i < worker_count; i++) {
        pthread_join(threads[i], NULL);
    }
    sync_destroy(&sync);

    const double elapsed = get_wall_seconds() - t0;
    const double updates = (double)((width > 2) ? (width - 2) : 0) *
                           (double)((height > 2) ? (height - 2) : 0) *
                           (double)steps;

    fprintf(stderr, "Threads: %d\n", total_threads);
    fprintf(stderr, "Elapsed time: %.6f seconds\n", elapsed);
    fprintf(stderr, "Updates/sec: %.3e\n",
            (elapsed > 0.0) ? (updates / elapsed) : 0.0);

    free(threads);
    free(args);
    free(grid);
    free(next);

    return 0;
}
