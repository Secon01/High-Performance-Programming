#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

/* Wall-clock timer (seconds). */
static inline double get_wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
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

    // Read AoS file format into temporary buffer, then de-interleave into SoA arrays.
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

    // SoA particle arrays (contiguous per attribute).
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

    // Thread-private acceleration contributions (size n_threads * N).
    double *ax_priv = (double*)malloc((size_t)n_threads * (size_t)N * sizeof(double));
    double *ay_priv = (double*)malloc((size_t)n_threads * (size_t)N * sizeof(double));
    if (!ax_priv || !ay_priv) {
        perror("malloc");
        free(ax_priv); free(ay_priv);
        free(ax); free(ay);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    double t_total0 = get_wall_seconds();
    double t_force = 0.0, t_update = 0.0;

    omp_set_num_threads(n_threads);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        double t0_local = 0.0;

        for (int step = 0; step < nsteps; step++) {

            // ---------------- Force phase (timed) ----------------
            #pragma omp barrier
            #pragma omp master
            { t0_local = get_wall_seconds(); }
            #pragma omp barrier

            // Zero this thread's private slice for the current timestep.
            for (int i = 0; i < N; i++) {
                ax_priv[(size_t)tid * (size_t)N + (size_t)i] = 0.0;
                ay_priv[(size_t)tid * (size_t)N + (size_t)i] = 0.0;
            }

            // Parallelize over i; compute each pair once (j > i).
            #pragma omp for schedule(static)
            for (int i = 0; i < N; i++) {
                double xi = x[i];
                double yi = y[i];
                double mi = m[i];

                // Accumulate i-contribution locally (write back once after inner loop).
                double axi = 0.0;
                double ayi = 0.0;

                for (int j = i + 1; j < N; j++) {
                    double xj = x[j];
                    double yj = y[j];
                    double mj = m[j];

                    double rx = xi - xj;
                    double ry = yi - yj;

                    double r2    = rx*rx + ry*ry;
                    double r     = sqrt(r2);
                    double denom = r + eps0;
                    double inv   = 1.0 / (denom * denom * denom);

                    double s_i = -G * mj * inv;
                    double s_j =  G * mi * inv;

                    // Keep i in registers; update j directly into private slice.
                    axi += s_i * rx;
                    ayi += s_i * ry;

                    ax_priv[(size_t)tid * (size_t)N + (size_t)j] += s_j * rx;
                    ay_priv[(size_t)tid * (size_t)N + (size_t)j] += s_j * ry;
                }

                // Write back i once after the j-loop (reduces hot-loop memory traffic).
                ax_priv[(size_t)tid * (size_t)N + (size_t)i] += axi;
                ay_priv[(size_t)tid * (size_t)N + (size_t)i] += ayi;
            }

            #pragma omp barrier
            #pragma omp master
            { t_force += get_wall_seconds() - t0_local; }

            // ---------------- Reduction phase (not timed) ----------------
            #pragma omp for schedule(static)
            for (int i = 0; i < N; i++) {
                double sx = 0.0, sy = 0.0;
                for (int t = 0; t < n_threads; t++) {
                    sx += ax_priv[(size_t)t * (size_t)N + (size_t)i];
                    sy += ay_priv[(size_t)t * (size_t)N + (size_t)i];
                }
                ax[i] = sx;
                ay[i] = sy;
            }

            // ---------------- Update phase (timed) ----------------
            #pragma omp barrier
            #pragma omp master
            { t0_local = get_wall_seconds(); }
            #pragma omp barrier

            #pragma omp for schedule(static)
            for (int i = 0; i < N; i++) {
                // v^{n+1} = v^n + dt*a^n
                vx[i] += dt * ax[i];
                vy[i] += dt * ay[i];

                // x^{n+1} = x^n + dt*v^{n+1}
                x[i]  += dt * vx[i];
                y[i]  += dt * vy[i];
            }

            #pragma omp barrier
            #pragma omp master
            { t_update += get_wall_seconds() - t0_local; }
        }
    } // end parallel region

    double t_total = get_wall_seconds() - t_total0;

    if (graphics == 0) {
        fprintf(stderr, "Timing:\n");
        fprintf(stderr, "  total  = %.6f s\n", t_total);
        fprintf(stderr, "  force  = %.6f s (%.1f%%)\n", t_force, 100.0 * t_force / t_total);
        fprintf(stderr, "  update = %.6f s (%.1f%%)\n", t_update, 100.0 * t_update / t_total);
    }

    // Interleave SoA arrays back to AoS format for output.
    double *outbuf = (double*)malloc(count * sizeof(double));
    if (!outbuf) {
        perror("malloc");
        free(ax_priv); free(ay_priv);
        free(ax); free(ay);
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
        free(ax_priv); free(ay_priv);
        free(ax); free(ay);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }
    size_t nwritten = fwrite(outbuf, sizeof(double), count, out);
    fclose(out);
    free(outbuf);

    if (nwritten != count) {
        fprintf(stderr, "Write error: expected %zu doubles, wrote %zu\n", count, nwritten);
        free(ax_priv); free(ay_priv);
        free(ax); free(ay);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    free(ax_priv); free(ay_priv);
    free(ax); free(ay);
    free(x); free(y); free(m); free(vx); free(vy); free(bb);

    return 0;
}