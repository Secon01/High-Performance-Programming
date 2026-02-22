#include "gol.h"    
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include "barrier.h"
// Print command-line usage
static void usage(const char *prog) {
    printf("Usage: %s [-W width] [-H height] [-s steps] [-t threads] [-p prob] [-seed N] [-print]\n", prog);
}
// Set wall-clock timer
double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}
// Thread arguments
typedef struct {
    int tid;                // thread id
    int start_row, end_row; // row range
    int W, H, steps;        // grid dimensions and timesteps
    int do_print;              // printing enabled flag (debug only)

    uint8_t **grid_ptr;   // pointer to shared pointer (grid)
    uint8_t **next_ptr;   // pointer to shared pointer (next)

    barrier_t *barrier;   // shared reusable barrier
} thread_arg_t;

// Worker thread: compute its rows each timestep and sync via barriers
static void *worker(void *argp) {
    thread_arg_t *a = (thread_arg_t *)argp;

    for (int t = 0; t < a->steps; t++) {
        uint8_t *grid = *a->grid_ptr;   // read shared curr pointer
        uint8_t *next = *a->next_ptr;   // read shared next pointer

        // Handle tiny grids with no interior (fixed-dead makes everything dead)
        if (a->W <= 2 || a->H <= 2) {
            if (a->tid == 0) fill_zero(next, a->W, a->H);  // only one thread writes entire grid
            barrier_wait(a->barrier);                      // ensure next is ready
        } else {
            if (a->tid == 0) set_borders_dead(next, a->W, a->H); // set fixed-dead borders once per step
            barrier_wait(a->barrier);                              // ensure borders are set

            step_range(grid, next, a->W, a->start_row, a->end_row); // compute assigned interior rows
            barrier_wait(a->barrier);                                     // ensure all rows are done
        }

        // One thread performs the pointer swap (double buffering)
        if (a->tid == 0) {
            uint8_t *tmp = *a->grid_ptr;
            *a->grid_ptr = *a->next_ptr;
            *a->next_ptr = tmp;
        }

        barrier_wait(a->barrier); // ensure swap is visible before next timestep

        // Optional debug printing (VERY slow; only for small grids)
        if (a->do_print && a->tid == 0) {
            print_grid(*a->grid_ptr, a->W, a->H);
            printf("----------------------\n");
        }
        barrier_wait(a->barrier); // keep threads in lockstep when printing is enabled
    }
    return NULL;
}

int main(int argc, char **argv)
{
    // Default parameters
    int W = 80, H = 40; // W(idth) number of columns, H(eight) number of rows 
    int steps = 1000;   // number of timesteps
    int nthreads = 1;   // number of pthreads
    double p = 0.30;    // initial alive probability     
    unsigned seed = 1;  // RNG seed
    int do_print = 0;   // print each timestep
    // Parse command line options
    for (int a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "-W") && a+1 < argc) W = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-H") && a+1 < argc) H = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-s") && a+1 < argc) steps = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-t") && a + 1 < argc) nthreads = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-p") && a+1 < argc) p = atof(argv[++a]);
        else if (!strcmp(argv[a], "-seed") && a+1 < argc) seed = (unsigned)atoi(argv[++a]);
        else if (!strcmp(argv[a], "-print")) do_print = 1;
        else { usage(argv[0]); return 1; }
    }

    // Basic argument sanity checks
    if (W <= 0 || H <= 0 || steps < 0 || nthreads <= 0 || p < 0.0 || p > 1.0) {
        fprintf(stderr, "Invalid arguments.\n");
        return 1;
    }

    // Allocate double buffers (curr and next)
    uint8_t *grid = (uint8_t *)malloc((size_t)W * (size_t)H * sizeof(uint8_t));
    uint8_t *next = (uint8_t *)malloc((size_t)W * (size_t)H * sizeof(uint8_t));
    if (!grid || !next) {
        perror("malloc");
        free(grid);
        free(next);
        return 1;
    }

    // Initialize grid with random alive/dead values
    srand(seed);
    fill_zero(grid, W, H);
    initialize_grid(p, grid, W, H);

    // Optional initial print
    if (do_print) {
        print_grid(grid, W, H);
        printf("-----Simulation-----\n");
    }

    // Serial version (no barrier needed)
    if (nthreads == 1) {
        double t0 = get_wall_seconds();

        for (int t = 0; t < steps; t++) {
            step(grid, next, W, H);             // compute next generation (serial)
            uint8_t *tmp = grid; grid = next; next = tmp; // swap buffers

            if (do_print) {
                print_grid(grid, W, H);
                printf("----------------------\n");
            }
        }

        double t1 = get_wall_seconds();
        double elapsed = t1 - t0;

        printf("Threads: %d\n", nthreads);
        printf("Elapsed time: %.6f seconds\n", elapsed);
        double updates = (double)W * (double)H * (double)steps;
        printf("Updates/sec: %.3e\n", updates / elapsed);

        free(grid);
        free(next);
        return 0;
    }

    // Parallel version (pthreads + custom barrier)
    pthread_t *threads = (pthread_t *)malloc((size_t)nthreads * sizeof(pthread_t));
    thread_arg_t *args = (thread_arg_t *)malloc((size_t)nthreads * sizeof(thread_arg_t));
    if (!threads || !args) {
        perror("malloc");
        free(threads);
        free(args);
        free(grid);
        free(next);
        return 1;
    }

    // Initialize reusable barrier for nthreads
    barrier_t barrier;
    if (barrier_init(&barrier, nthreads) != 0) {
        fprintf(stderr, "barrier_init failed\n");
        free(threads);
        free(args);
        free(grid);
        free(next);
        return 1;
    }

    // Shared pointers so thread 0 can swap and everyone sees it
    uint8_t *shared_grid = grid;
    uint8_t *shared_next = next;

    // Split only interior rows among threads: rows [1, H-1)
    int interior_start = 1;
    int interior_end = (H > 1) ? (H - 1) : 1;
    int interior_rows = interior_end - interior_start; // may be <= 0 for small H

    int base = (interior_rows > 0) ? (interior_rows / nthreads) : 0;
    int rem  = (interior_rows > 0) ? (interior_rows % nthreads) : 0;
    printf("H=%d interior_rows=%d base=%d rem=%d\n", H, interior_rows, base, rem);
    int r = interior_start;

    // Start timing (includes thread work; excludes allocation/init)
    double t0 = get_wall_seconds();

    // Create threads
    for (int tid = 0; tid < nthreads; tid++) {
        int take = base + (tid < rem ? 1 : 0);       // distribute remainder rows
        int r0 = r;
        int r1 = r + take;
        r = r1;

        args[tid].tid = tid;
        args[tid].start_row = r0;
        args[tid].end_row = r1;
        args[tid].W = W;
        args[tid].H = H;
        args[tid].steps = steps;
        args[tid].do_print = do_print;

        args[tid].grid_ptr = &shared_grid;
        args[tid].next_ptr = &shared_next;
        args[tid].barrier = &barrier;
        printf("tid %d rows [%d, %d)\n", tid, args[tid].start_row, args[tid].end_row);

        if (pthread_create(&threads[tid], NULL, worker, &args[tid]) != 0) {
            fprintf(stderr, "pthread_create failed for tid=%d\n", tid);
            // If a thread fails to start, others may deadlock; best exit early in student project
            return 1;
        }
    }

    // Join threads
    for (int tid = 0; tid < nthreads; tid++) {
        pthread_join(threads[tid], NULL);
    }

    double t1 = get_wall_seconds();
    double elapsed = t1 - t0;
    // Clean up barrier resources
    barrier_destroy(&barrier);

    // Report timing
    printf("Threads: %d\n", nthreads);
    printf("Elapsed time: %.6f seconds\n", elapsed);
    double updates = (double)(W - 2) * (double)(H - 2) * (double)steps;
    printf("Updates/sec: %.3e\n", updates / elapsed);

    // Free memory
    free(threads);
    free(args);
    free(shared_grid);
    free(shared_next);

    return 0;
}