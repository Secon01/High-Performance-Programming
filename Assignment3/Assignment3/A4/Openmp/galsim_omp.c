#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>        // OpenMP header

enum { X=0, Y=1, M=2, VX=3, VY=4, B=5 };    // indices for data[6*i + k] layout

static inline double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
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
    int n_threads = atoi(argv[6]);      // number of threads

    if (N <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }               // validate N
    if (nsteps < 0) { fprintf(stderr, "nsteps must be >= 0\n"); return 1; }     // validate n steps

    size_t count = (size_t)6 * (size_t)N;                 // count the numbers per particle
    double *data = malloc(count * sizeof(double));        // particle's data array
    if (!data) {                                          // malloc check
        perror("malloc");
        return 1;
    }

    // Read binary file of galaxy particles
    FILE *input = fopen(filename, "rb");
    if (!input) {                                         // check if binary file is opened
        perror("fopen input");
        free(data);
        return 1;
    }

    size_t nread = fread(data, sizeof(double), count, input);  // read the numbers
    fclose(input);

    if (nread != count) {                                 // check if the numbers of the binary file are of the correct size
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

    // Tell OpenMP to use the requested number of threads
    omp_set_num_threads(n_threads);

    double t_total0 = get_wall_seconds();   // total time
    double t_force = 0.0;                   // force
    double t_update = 0.0;                  // update velocities, positions

    // Use one persistent parallel region to avoid per-phase thread creation overhead
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();     // worker's id (OpenMP thread id)
        double t0_local = 0.0;              // local timing helper for master timing

        for (int step = 0; step < nsteps; step++)
        {
            // ---------------- Force phase (timed) ----------------

            #pragma omp barrier              // ensure all threads are ready before timing starts
            #pragma omp master
            { t0_local = get_wall_seconds(); }   // start force timer (single thread)
            #pragma omp barrier              // align start time for all threads

            // Zero only this worker's private slice for this timestep (ax_priv index: tid*N + i)
            for (int i = 0; i < N; i++) {
                ax_priv[(size_t)tid * (size_t)N + (size_t)i] = 0.0;
                ay_priv[(size_t)tid * (size_t)N + (size_t)i] = 0.0;
            }

            // Compute pair interactions in parallel over i, with j = i+1..N-1
            #pragma omp for schedule(static)
            for (int i = 0; i < N; i++) {
                // Read particle i's position and mass
                double xi = data[6*i + X];
                double yi = data[6*i + Y];
                double mi = data[6*i + M];

                // Only compute each pair once: j > i
                for (int j = i + 1; j < N; j++) {
                    // Read particle j's position and mass
                    double xj = data[6*j + X];
                    double yj = data[6*j + Y];
                    double mj = data[6*j + M];

                    // Vector between particles: r_ij = (xi-xj, yi-yj)
                    double rx = xi - xj;
                    double ry = yi - yj;

                    // Distance between particles (in 2D)
                    double r = sqrt(rx*rx + ry*ry);
                    double denom = r + eps0;
                    double inv = 1.0 / (denom * denom * denom);

                    // Update i and j using equal and opposite interaction (Newton’s third law)
                    double s_i = -G * mj * inv;
                    double s_j =  G * mi * inv;

                    // update i contribution in worker's private slice
                    ax_priv[(size_t)tid * (size_t)N + (size_t)i] += s_i * rx;
                    ay_priv[(size_t)tid * (size_t)N + (size_t)i] += s_i * ry;

                    // update j contribution in worker's private slice
                    ax_priv[(size_t)tid * (size_t)N + (size_t)j] += s_j * rx;
                    ay_priv[(size_t)tid * (size_t)N + (size_t)j] += s_j * ry;
                }
            }

            #pragma omp barrier              // ensure force phase is fully finished before stopping timer
            #pragma omp master
            { t_force += get_wall_seconds() - t0_local; }  // accumulate force time (single thread)

            // ---------------- Reduction phase (not timed) ----------------

            // Reduce thread-private accelerations into global ax/ay for a chunk of particles
            #pragma omp for schedule(static)
            for (int i = 0; i < N; i++) {
                double sx = 0.0;
                double sy = 0.0;

                // Sum contributions for particle i across all workers' private slices (t*N + i)
                for (int t = 0; t < n_threads; t++) {
                    sx += ax_priv[(size_t)t * (size_t)N + (size_t)i];
                    sy += ay_priv[(size_t)t * (size_t)N + (size_t)i];
                }

                // Store the final reduced acceleration for particle i into the global arrays
                ax[i] = sx;
                ay[i] = sy;
            }

            // ---------------- Update phase (timed) ----------------

            #pragma omp barrier              // ensure all threads are ready before timing starts
            #pragma omp master
            { t0_local = get_wall_seconds(); }   // start update timer (single thread)
            #pragma omp barrier              // align start time for all threads

            // Update velocities and positions (symplectic Euler) using global ax/ay
            #pragma omp for schedule(static)
            for (int i = 0; i < N; i++) {
                // Velocity update
                data[6*i + VX] += dt * ax[i];
                data[6*i + VY] += dt * ay[i];

                // Position update
                data[6*i + X]  += dt * data[6*i + VX];
                data[6*i + Y]  += dt * data[6*i + VY];
            }

            #pragma omp barrier              // ensure update phase is fully finished before stopping timer
            #pragma omp master
            { t_update += get_wall_seconds() - t0_local; }  // accumulate update time (single thread)
        }
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
    return 0;
}