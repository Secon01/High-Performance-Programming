#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char **argv)
{
    if (argc != 6) {    // check input arguments
        fprintf(stderr, "Usage: %s N filename nsteps delta_t graphics\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);              // number of stars/particles
    const char *filename = argv[2];     // filename of the file to read the initial configuration from
    int nsteps = atoi(argv[3]);         // number of timesteps
    double dt = atof(argv[4]);          // timestep ∆t
    int graphics = atoi(argv[5]);       // graphics on/oﬀ.
    if (N <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }               // validate N
    if (nsteps < 0) { fprintf(stderr, "nsteps must be >= 0\n"); return 1; }     // validate n steps
    (void)graphics;                     // ignore for now

    size_t count = (size_t)6 * (size_t)N;               // count the numbers per particle

    // Use SoA (Structure of Arrays) to improve auto-vectorization.
    double *x  = malloc((size_t)N * sizeof(double));
    double *y  = malloc((size_t)N * sizeof(double));
    double *m  = malloc((size_t)N * sizeof(double));
    double *vx = malloc((size_t)N * sizeof(double));
    double *vy = malloc((size_t)N * sizeof(double));
    double *bb = malloc((size_t)N * sizeof(double));   // brightness
    if (!x || !y || !m || !vx || !vy || !bb) {  // malloc check
        perror("malloc");
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    // Read binary file of galaxy particles
    FILE *input = fopen(filename, "rb");
    if (!input) {   // check if binary file is opened
        perror("fopen input");
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    // Read as AoS into a temporary buffer, then de-interleave to SoA arrays.
    double *data = malloc(count * sizeof(double));    // particle's data array
    if(!data){  // malloc check
        perror("malloc");
        fclose(input);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    size_t nread = fread(data, sizeof(double), count, input);    // read the numbers
    fclose(input);

    if (nread != count) {  // check if the numbers of the binary file are of the correct size
        fprintf(stderr, "Read error: expected %zu doubles, got %zu\n", count, nread);
        free(data);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    enum { X=0, Y=1, M=2, VX=3, VY=4, B=5 };    // indices for data[6*i + k] layout

    // De-interleave AoS -> SoA once (faster inner loops, easier vectorization).
    for (int i = 0; i < N; i++) {
        int base = 6 * i;                 // indexing computed once per i (not inside hot loops)
        x[i]  = data[base + X];
        y[i]  = data[base + Y];
        m[i]  = data[base + M];
        vx[i] = data[base + VX];
        vy[i] = data[base + VY];
        bb[i] = data[base + B];
    }
    free(data);                            // no longer needed after de-interleave

    // One timestep: 1) Compute Acceleration, 2) Update velocities, Update positions (symplectic Euler)
    double G = 100.0 / (double)N;   // gravity scales with 1/N
    double eps0 = 1e-3;             // softening term to avoid singular forces
    // Temporary arrays to store accelerations for each particle
    double *ax = calloc((size_t)N, sizeof(double));
    double *ay = calloc((size_t)N, sizeof(double));
    if (!ax || !ay) {
        perror("calloc");
        free(ax); free(ay);
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }
    // Timestep loop
    for (int step = 0; step < nsteps; step++)
    {
        // Compute accelerations (symmetry / Newton's 3rd law)
        for (int i = 0; i < N; i++) {
            // Reset accumulated accelerations for particle i
            ax[i] = 0.0;
            ay[i] = 0.0;
        }
        #ifdef BASELINE
        // BASELINE: compute all j != i (no symmetry optimization)
        for (int i = 0; i < N; i++) {
            double xi = x[i];
            double yi = y[i];

            double axi = 0.0;
            double ayi = 0.0;

            for (int j = 0; j < N; j++) {
                if (j == i) continue;

                double rx = xi - x[j];
                double ry = yi - y[j];

                double r2 = rx*rx + ry*ry;
                double r  = sqrt(r2);
                double denom = r + eps0;
                double inv = 1.0 / (denom * denom * denom);

                double s = -G * m[j] * inv;

                axi += s * rx;
                ayi += s * ry;
            }
            ax[i] = axi;
            ay[i] = ayi;
        }
        #else
        for (int i = 0; i < N; i++) {
            // Read particle i's position and mass
            // Load these once per i (outside inner loop) to avoid repeated indexing work.
            double xi = x[i];
            double yi = y[i];
            double mi = m[i];
            // Accumulate into local scalars
            double axi = 0.0;
            double ayi = 0.0;
            // Only compute each pair once: j > i
            for (int j = i + 1; j < N; j++) {
                // Read particle j's position and mass
                double xj = x[j];
                double yj = y[j];
                double mj = m[j];
                // Vector between particles: r_ij = (xi-xj, yi-yj)
                double rx = xi - xj;
                double ry = yi - yj;
                // Distance between particles (in 2D)
                double r2 = rx*rx + ry*ry;              // compute squared distance once
                double r = sqrt(r2);
                double denom = r + eps0;
                double inv = 1.0 / (denom * denom * denom); // 1/(r+eps)^3
                // Update i and j using equal and opposite interaction
                double s_i = -G * mj * inv;
                double s_j =  G * mi * inv;
                // NEW: update i locally (write once after j loop)
                axi += s_i * rx;
                ayi += s_i * ry;
                ax[j] += s_j * rx;
                ay[j] += s_j * ry;
            }
            // Write back once after the j loop
            ax[i] += axi;
            ay[i] += ayi;
        }
        #endif
        // Update velocities and positions
        for (int i = 0; i < N; i++) {
            vx[i] += dt * ax[i];
            vy[i] += dt * ay[i];

            x[i]  += dt * vx[i];
            y[i]  += dt * vy[i];
        }
    }
    // Clean up temporary arrays
    free(ax);
    free(ay);

    // Write the partciles' numbers back
    FILE *out = fopen("result.gal", "wb");
    if (!out) {
        perror("fopen output");
        free(x); free(y); free(m); free(vx); free(vy); free(bb);
        return 1;
    }

    // Re-interleave SoA -> AoS on output to keep the same binary format.
    double buf[6];
    for (int i = 0; i < N; i++) {
        buf[X]  = x[i];
        buf[Y]  = y[i];
        buf[M]  = m[i];
        buf[VX] = vx[i];
        buf[VY] = vy[i];
        buf[B]  = bb[i];

        size_t nw = fwrite(buf, sizeof(double), 6, out);     // write the numbers
        if (nw != 6) {    // check if the size of numbers written is correct
            fprintf(stderr, "Write error: expected %d doubles, wrote %zu\n", 6, nw);
            fclose(out);
            free(x); free(y); free(m); free(vx); free(vy); free(bb);
            return 1;
        }
    }
    fclose(out);

    free(x);
    free(y);
    free(m);
    free(vx);
    free(vy);
    free(bb);

    //printf("Wrote result.gal successfully.\n");
    return 0;
}