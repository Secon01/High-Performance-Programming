#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>   

enum { X=0, Y=1, M=2, VX=3, VY=4, B=5 };    // indices for data[6*i + k] layout
static inline double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

// Thread argument struct
typedef struct {
    int tid;            // worker's id
    int n_threads;      // number of threads
    int N;              // number of particles
    double dt;
    double G;           // gravity
    double eps0;        // softening term to avoid singular forces
    double *data;       // your 6*N array (pointer to the particle data)
    double *ax, *ay;    // global acceleration arrays
    double *ax_priv;    // size n_threads*N
    double *ay_priv;    // size n_threads*N
} thread_args_t;

// Particles' partioning for each thread
static inline void get_range(int tid, int n_threads, int N, int *start, int *end)
{
    int chunk = (N + n_threads - 1) / n_threads; // ceil(N/n_threads)
    *start = tid * chunk;
    *end = *start + chunk;
    if (*end > N) *end = N;
}

// Parallel force phase: compute pairwise accelerations for this worker's i-range into its private slice.
void *worker_force(void *argp)
{
    thread_args_t *a = (thread_args_t*)argp;    // cast to thread_arg type
    int N = a->N;                               // initialize number of particles
    int tid = a->tid;                           // initialize worker's id

    int i0, i1;                                 
    get_range(tid, a->n_threads, N, &i0, &i1);  // get particles' partition for worker
    // Zero only this worker's private slice for this timestep
    // ax_priv index: tid*N + i
    for (int i = 0; i < N; i++) {
        a->ax_priv[(size_t)tid * N + i] = 0.0;
        a->ay_priv[(size_t)tid * N + i] = 0.0;
    }
    // Compute pair interactions for i in this worker's range, j = i+1..N-1 
    for (int i = i0; i < i1; i++) {
        // Read particle i's position and mass
        double xi = a->data[6*i + X];
        double yi = a->data[6*i + Y];
        double mi = a->data[6*i + M];
        // Only compute each pair once: j > i
        for (int j = i + 1; j < N; j++) {
            // Read particle j's position and mass
            double xj = a->data[6*j + X];
            double yj = a->data[6*j + Y];
            double mj = a->data[6*j + M];
            // Vector between particles: r_ij = (xi-xj, yi-yj)
            double rx = xi - xj;
            double ry = yi - yj;
            // Distance between particles (in 2D)
            double r = sqrt(rx*rx + ry*ry);
            double denom = r + a->eps0;
            double inv = 1.0 / (denom * denom * denom);
            // Update i and j using equal and opposite interaction (Newton’s third law)
            double s_i = -a->G * mj * inv;
            double s_j =  a->G * mi * inv;
            // update i contribution in worker's private slice
            a->ax_priv[(size_t)tid * N + i] += s_i * rx;
            a->ay_priv[(size_t)tid * N + i] += s_i * ry;
            // update j contribution in worker's private slice
            a->ax_priv[(size_t)tid * N + j] += s_j * rx;
            a->ay_priv[(size_t)tid * N + j] += s_j * ry;
        }
    }
    return NULL;
}
// Reduce thread-private accelerations into global ax/ay for this worker's particle range.
void *worker_reduce(void *argp)
{
    thread_args_t *a = (thread_args_t*)argp;
    int N = a->N;
    // Compute this worker's [i0,i1) chunk of particle indices for the reduction.
    int i0, i1;
    get_range(a->tid, a->n_threads, N, &i0, &i1);
    // Sum contributions for particle i across all workers' private slices (t*N + i).
    for (int i = i0; i < i1; i++) {
        double sx = 0.0;
        double sy = 0.0;
        // Sum contributions for particle i from every worker's private slices
        for (int t = 0; t < a->n_threads; t++) {
            sx += a->ax_priv[(size_t)t * N + i];
            sy += a->ay_priv[(size_t)t * N + i];
        }
        // Store the final reduced acceleration for particle i into the global arrays.
        a->ax[i] = sx;
        a->ay[i] = sy;
    }
    return NULL;
}
// Update velocities and positions (symplectic Euler) for this worker's particle range using global ax/ay.
void *worker_update(void *argp)
{
    thread_args_t *a = (thread_args_t*)argp;
    int N = a->N;
    // Compute this worker's [i0,i1) chunk of particle indices for the update.
    int i0, i1;
    get_range(a->tid, a->n_threads, N, &i0, &i1);
    for (int i = i0; i < i1; i++) {
        // Velocity update
        a->data[6*i + VX] += a->dt * a->ax[i];
        a->data[6*i + VY] += a->dt * a->ay[i];

        // Position update
        a->data[6*i + X]  += a->dt * a->data[6*i + VX];
        a->data[6*i + Y]  += a->dt * a->data[6*i + VY];
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 7) {    // check input arguments
        fprintf(stderr, "Usage: %s N filename nsteps delta_t graphics n_threads\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);              // number of stars/particles 
    const char *filename = argv[2];     // filename of the file to read the initial configuration from
    int nsteps = atoi(argv[3]);         // number of timesteps
    double dt = atof(argv[4]);          // timestep ∆t
    int graphics = atoi(argv[5]);       // graphics on/oﬀ.
    int n_threads = atoi(argv[6]);           // number of threads
    if (N <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }               // validate N
    if (nsteps < 0) { fprintf(stderr, "nsteps must be >= 0\n"); return 1; }     // validate n steps
    (void)graphics;                     // ignore for now 
    size_t count = (size_t)6 * (size_t)N;               // count the numbers per particle
    double *data = malloc(count * sizeof(double));    // particle's data array
    if(!data){  // malloc check
        perror("malloc");
        return 1;
    }
    // Read binary file of galaxy particles
    FILE *input = fopen(filename, "rb");
    if (!input) {   // check if binary file is opened
        perror("fopen input");
        free(data);
        return 1;
    }
    size_t nread = fread(data, sizeof(double), count, input);    // read the numbers
    fclose(input);
    
     if (nread != count) {  // check if the numbers of the binary file are of the correct size
        fprintf(stderr, "Read error: expected %zu doubles, got %zu\n", count, nread);
        free(data);
        return 1;
    }
    // One timestep: 1) Compute Acceleration, 2) Update velocities, Update positions (symplectic Euler)
    double G = 100.0 / (double)N;   // gravity scales with 1/N
    double eps0 = 1e-3;             // softening term to avoid singular forces
    // Temporary arrays to store accelerations for each particle
    double *ax = calloc((size_t)N, sizeof(double));
    double *ay = calloc((size_t)N, sizeof(double));
    if (!ax || !ay) {
        perror("calloc");
        free(data);
        free(ax); free(ay);
        return 1;
    }
    // Thread-private acceleration arrays: one slice of size N per thread
    double *ax_priv = malloc((size_t)n_threads * (size_t)N * sizeof(double));
    double *ay_priv = malloc((size_t)n_threads * (size_t)N * sizeof(double));
    if (!ax_priv || !ay_priv) {
        perror("malloc");
        free(data);
        free(ax); free(ay);
        free(ax_priv); free(ay_priv);
        return 1;
    }
    // Create threads and thread arguments arrays
    pthread_t *threads = malloc((size_t)n_threads * sizeof(pthread_t));
    thread_args_t *args = malloc((size_t)n_threads * sizeof(thread_args_t));
    if (!threads || !args) {
        perror("malloc");
        free(data);
        free(ax); free(ay);
        free(ax_priv); free(ay_priv);
        free(threads); free(args);
        return 1;
    }
    // Populate thread arguments
    for (int t = 0; t < n_threads; t++) {
        args[t].tid = t;
        args[t].n_threads = n_threads;
        args[t].N = N;
        args[t].dt = dt;

        args[t].G = G;
        args[t].eps0 = eps0;

        args[t].data = data;
        args[t].ax = ax;
        args[t].ay = ay;
        args[t].ax_priv = ax_priv;
        args[t].ay_priv = ay_priv;
    }
    double t_total0 = get_wall_seconds();   // total time
    double t_force = 0.0;                   // force
    double t_update = 0.0;                  // update velocities, positions
    for (int step = 0; step < nsteps; step++)
    {
        // Force phase
        double t0 = get_wall_seconds();
        for (int t = 0; t < n_threads; t++) {
            pthread_create(&threads[t], NULL, worker_force, &args[t]);
        }
        for (int t = 0; t < n_threads; t++) {
            pthread_join(threads[t], NULL);
        }
        t_force += get_wall_seconds() - t0;
        // Reduction phase
        for (int t = 0; t < n_threads; t++) {
            pthread_create(&threads[t], NULL, worker_reduce, &args[t]);
        }
        for (int t = 0; t < n_threads; t++) {
            pthread_join(threads[t], NULL);
        }
        // Update phase
        t0 = get_wall_seconds();
        for (int t = 0; t < n_threads; t++) {
            pthread_create(&threads[t], NULL, worker_update, &args[t]);
        }
        for (int t = 0; t < n_threads; t++) {
            pthread_join(threads[t], NULL);
        }
        t_update += get_wall_seconds() - t0;
    }    
    double t_total = get_wall_seconds() - t_total0;
    // Print the 3 timings 
    if (graphics == 0) {
        fprintf(stderr, "Timing:\n");
        fprintf(stderr, "  total  = %.6f s\n", t_total);
        fprintf(stderr, "  force  = %.6f s (%.1f%%)\n", t_force, 100.0 * t_force / t_total);
        fprintf(stderr, "  update = %.6f s (%.1f%%)\n", t_update, 100.0 * t_update / t_total);
    }
    // Clean up temporary arrays
    free(ax);
    free(ay);
    free(ax_priv);
    free(ay_priv);
    free(threads);
    free(args);

    // Write the partciles' numbers back
    FILE *out = fopen("result.gal", "wb");
    if (!out) {
        perror("fopen output");
        free(data);
        return 1;
    }
    size_t nwritten = fwrite(data, sizeof(double), count, out);     // write the numbers
    fclose(out);
    if (nwritten != count) {    // check if the size of numbers written is correct
        fprintf(stderr, "Write error: expected %zu doubles, wrote %zu\n", count, nwritten);
        free(data);
        return 1;
    }
    free(data);
    //printf("Wrote result.gal successfully.\n");
    return 0;
}