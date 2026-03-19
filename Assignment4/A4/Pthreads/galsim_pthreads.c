#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

/* Wall-clock timer (seconds). */
static inline double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

/* Arguments passed to each pthread worker. */
typedef struct {
    int tid;                 // thread id
    int n_threads;           // total number of threads
    int N;                   // number of particles
    double dt;               // time step
    double G;                // gravitational constant (100/N)
    double eps0;             // Plummer softening

    // SoA particle arrays (contiguous per attribute)
    double *x, *y, *m, *vx, *vy, *bb;

    // Global acceleration arrays (after reduction)
    double *ax, *ay;

    // Thread-private acceleration contributions (size n_threads * N)
    double *ax_priv;
    double *ay_priv;
} thread_args_t;

/* Static block partitioning for indices [0, N). */
static inline void get_range(int tid, int n_threads, int N, int *start, int *end)
{
    int chunk = (N + n_threads - 1) / n_threads;  // ceil(N/n_threads)
    *start = tid * chunk;
    *end   = *start + chunk;
    if (*end > N) *end = N;
}

/* Force phase: compute pairwise interactions for this thread's i-range into its private slice. */
static void *worker_force(void *argp)
{
    thread_args_t *a = (thread_args_t*)argp;
    int N   = a->N;
    int tid = a->tid;

    int i0, i1;
    get_range(tid, a->n_threads, N, &i0, &i1);

    // Zero this thread's private slice for the current timestep.
    for (int i = 0; i < N; i++) {
        a->ax_priv[(size_t)tid * (size_t)N + (size_t)i] = 0.0;
        a->ay_priv[(size_t)tid * (size_t)N + (size_t)i] = 0.0;
    }

    // Compute each pair once (j > i); store contributions into thread-private arrays.
    for (int i = i0; i < i1; i++) {
        double xi = a->x[i];
        double yi = a->y[i];
        double mi = a->m[i];

        // Accumulate i-contribution locally (write back once after the inner loop).
        double axi = 0.0;
        double ayi = 0.0;

        for (int j = i + 1; j < N; j++) {
            double xj = a->x[j];
            double yj = a->y[j];
            double mj = a->m[j];

            double rx = xi - xj;
            double ry = yi - yj;

            double r2    = rx*rx + ry*ry;
            double r     = sqrt(r2);
            double denom = r + a->eps0;
            double inv   = 1.0 / (denom * denom * denom);  // 1/(r+eps)^3

            // Equal and opposite updates (Newton's third law).
            double s_i = -a->G * mj * inv;
            double s_j =  a->G * mi * inv;

            // Keep i in registers; update j directly into private slice.
            axi += s_i * rx;
            ayi += s_i * ry;

            a->ax_priv[(size_t)tid * (size_t)N + (size_t)j] += s_j * rx;
            a->ay_priv[(size_t)tid * (size_t)N + (size_t)j] += s_j * ry;
        }

        // Write back i once after the j-loop (reduces hot-loop memory traffic).
        a->ax_priv[(size_t)tid * (size_t)N + (size_t)i] += axi;
        a->ay_priv[(size_t)tid * (size_t)N + (size_t)i] += ayi;
    }

    return NULL;
}

/* Reduction phase: sum all thread-private contributions into global ax/ay for this thread's i-range. */
static void *worker_reduce(void *argp)
{
    thread_args_t *a = (thread_args_t*)argp;
    int N = a->N;

    int i0, i1;
    get_range(a->tid, a->n_threads, N, &i0, &i1);

    for (int i = i0; i < i1; i++) {
        double sx = 0.0, sy = 0.0;
        for (int t = 0; t < a->n_threads; t++) {
            sx += a->ax_priv[(size_t)t * (size_t)N + (size_t)i];
            sy += a->ay_priv[(size_t)t * (size_t)N + (size_t)i];
        }
        a->ax[i] = sx;
        a->ay[i] = sy;
    }

    return NULL;
}

/* Update phase: symplectic Euler step for this thread's i-range. */
static void *worker_update(void *argp)
{
    thread_args_t *a = (thread_args_t*)argp;
    int N = a->N;

    int i0, i1;
    get_range(a->tid, a->n_threads, N, &i0, &i1);

    for (int i = i0; i < i1; i++) {
        // v^{n+1} = v^n + dt*a^n
        a->vx[i] += a->dt * a->ax[i];
        a->vy[i] += a->dt * a->ay[i];

        // x^{n+1} = x^n + dt*v^{n+1}
        a->x[i]  += a->dt * a->vx[i];
        a->y[i]  += a->dt * a->vy[i];
    }

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 7) {
        fprintf(stderr, "Usage: %s N filename nsteps delta_t graphics n_threads\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    const char *filename = argv[2];
    int nsteps = atoi(argv[3]);
    double dt = atof(argv[4]);
    int graphics = atoi(argv[5]);
    int n_threads = atoi(argv[6]);

    if (N <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }
    if (nsteps < 0) { fprintf(stderr, "nsteps must be >= 0\n"); return 1; }
    if (n_threads <= 0) { fprintf(stderr, "n_threads must be > 0\n"); return 1; }
    (void)graphics; // graphics ignored for performance measurements

    size_t count = (size_t)6 * (size_t)N;

    // Read AoS file format into a temporary buffer, then de-interleave into SoA arrays.
    double *buf = (double*)malloc(count * sizeof(double));
    if (!buf) { perror("malloc"); return 1; }

    FILE *input = fopen(filename, "rb");
    if (!input) { perror("fopen input"); free(buf); return 1; }

    size_t nread = fread(buf, sizeof(double), count, input);
    fclose(input);

    if (nread != count) {
        fprintf(stderr, "Read error: expected %zu doubles, got %zu\n", count, nread);
        free(buf);
        return 1;
    }

    // SoA particle arrays.
    double *x  = (double*)malloc((size_t)N * sizeof(double));
    double *y  = (double*)malloc((size_t)N * sizeof(double));
    double *m  = (double*)malloc((size_t)N * sizeof(double));
    double *vx = (double*)malloc((size_t)N * sizeof(double));
    double *vy = (double*)malloc((size_t)N * sizeof(double));
    double *bb = (double*)malloc((size_t)N * sizeof(double));
    if (!x || !y || !m || !vx || !vy || !bb) {
        perror("malloc");
        free(buf);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    // De-interleave AoS buffer -> SoA arrays (keeps file format unchanged).
    for (int i = 0; i < N; i++) {
        x[i]  = buf[6*(size_t)i + 0];
        y[i]  = buf[6*(size_t)i + 1];
        m[i]  = buf[6*(size_t)i + 2];
        vx[i] = buf[6*(size_t)i + 3];
        vy[i] = buf[6*(size_t)i + 4];
        bb[i] = buf[6*(size_t)i + 5];
    }
    free(buf);

    double G = 100.0 / (double)N;
    double eps0 = 1e-3;

    // Global accelerations (after reduction).
    double *ax = (double*)malloc((size_t)N * sizeof(double));
    double *ay = (double*)malloc((size_t)N * sizeof(double));
    if (!ax || !ay) {
        perror("malloc");
        free(ax); free(ay);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    // Thread-private acceleration storage.
    double *ax_priv = (double*)malloc((size_t)n_threads * (size_t)N * sizeof(double));
    double *ay_priv = (double*)malloc((size_t)n_threads * (size_t)N * sizeof(double));
    if (!ax_priv || !ay_priv) {
        perror("malloc");
        free(ax); free(ay);
        free(ax_priv); free(ay_priv);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    pthread_t *threads = (pthread_t*)malloc((size_t)n_threads * sizeof(pthread_t));
    thread_args_t *args = (thread_args_t*)malloc((size_t)n_threads * sizeof(thread_args_t));
    if (!threads || !args) {
        perror("malloc");
        free(threads); free(args);
        free(ax); free(ay);
        free(ax_priv); free(ay_priv);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    for (int t = 0; t < n_threads; t++) {
        args[t].tid = t;
        args[t].n_threads = n_threads;
        args[t].N = N;
        args[t].dt = dt;
        args[t].G = G;
        args[t].eps0 = eps0;

        args[t].x = x; args[t].y = y; args[t].m = m;
        args[t].vx = vx; args[t].vy = vy; args[t].bb = bb;

        args[t].ax = ax; args[t].ay = ay;
        args[t].ax_priv = ax_priv; args[t].ay_priv = ay_priv;
    }

    double t_total0 = get_wall_seconds();
    double t_force = 0.0, t_update = 0.0;

    for (int step = 0; step < nsteps; step++) {
        // Force phase (timed)
        double t0 = get_wall_seconds();
        for (int t = 0; t < n_threads; t++) pthread_create(&threads[t], NULL, worker_force, &args[t]);
        for (int t = 0; t < n_threads; t++) pthread_join(threads[t], NULL);
        t_force += get_wall_seconds() - t0;

        // Reduction phase (not timed)
        for (int t = 0; t < n_threads; t++) pthread_create(&threads[t], NULL, worker_reduce, &args[t]);
        for (int t = 0; t < n_threads; t++) pthread_join(threads[t], NULL);

        // Update phase (timed)
        t0 = get_wall_seconds();
        for (int t = 0; t < n_threads; t++) pthread_create(&threads[t], NULL, worker_update, &args[t]);
        for (int t = 0; t < n_threads; t++) pthread_join(threads[t], NULL);
        t_update += get_wall_seconds() - t0;
    }

    double t_total = get_wall_seconds() - t_total0;

    if (graphics == 0) {
        fprintf(stderr, "Timing:\n");
        fprintf(stderr, "  total  = %.6f s\n", t_total);
        fprintf(stderr, "  force  = %.6f s (%.1f%%)\n", t_force, 100.0 * t_force / t_total);
        fprintf(stderr, "  update = %.6f s (%.1f%%)\n", t_update, 100.0 * t_update / t_total);
    }

    // Interleave SoA arrays back to AoS file format for output.
    double *outbuf = (double*)malloc(count * sizeof(double));
    if (!outbuf) {
        perror("malloc");
        free(ax); free(ay); free(ax_priv); free(ay_priv);
        free(threads); free(args);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }
    for (int i = 0; i < N; i++) {
        outbuf[6*(size_t)i + 0] = x[i];
        outbuf[6*(size_t)i + 1] = y[i];
        outbuf[6*(size_t)i + 2] = m[i];
        outbuf[6*(size_t)i + 3] = vx[i];
        outbuf[6*(size_t)i + 4] = vy[i];
        outbuf[6*(size_t)i + 5] = bb[i];
    }

    FILE *out = fopen("result.gal", "wb");
    if (!out) {
        perror("fopen output");
        free(outbuf);
        free(ax); free(ay); free(ax_priv); free(ay_priv);
        free(threads); free(args);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }
    size_t nwritten = fwrite(outbuf, sizeof(double), count, out);
    fclose(out);
    free(outbuf);

    if (nwritten != count) {
        fprintf(stderr, "Write error: expected %zu doubles, wrote %zu\n", count, nwritten);
        free(ax); free(ay); free(ax_priv); free(ay_priv);
        free(threads); free(args);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    free(ax); free(ay);
    free(ax_priv); free(ay_priv);
    free(threads); free(args);
    free(x); free(y); free(m); free(vx); free(vy); free(bb);

    return 0;
}