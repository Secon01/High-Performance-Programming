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
    
    // Print particle 0
    //printf("Particle 0:\n");
    //printf("  x=%g\n  y=%g\n  m=%g\n  vx=%g\n  vy=%g\n  b=%g\n",
    //       data[0], data[1], data[2], data[3], data[4], data[5]);

    enum { X=0, Y=1, M=2, VX=3, VY=4, B=5 };    // indices for data[6*i + k] layout
    
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
    // Timestep loop
    for (int step = 0; step < nsteps; step++)
    {
        /*// Compute accelerations
        for (int i = 0; i < N; i++) {
            // Read particle i's position
            double xi = data[6*i + X];
            double yi = data[6*i + Y];
            // Accumulated acceleration for particle i
            double axi = 0.0;
            double ayi = 0.0;
            // Sum contribution from every other particle j
            for (int j = 0; j < N; j++) {
                if (j == i) continue;       // a particle does not attract itself
                // Read particle j position and mass
                double xj = data[6*j + X];
                double yj = data[6*j + Y];
                double mj = data[6*j + M];
                // Vector from j to i: r_ij = (xi-xj, yi-yj)
                double rx = xi - xj;
                double ry = yi - yj;
                // Distance between particles (in 2D)
                double r = sqrt(rx*rx + ry*ry);
                double denom = r + eps0;
                double inv   = 1.0 / (denom * denom * denom);  // 1 / (r+eps)^3
                axi += -G * mj * inv * rx;
                ayi += -G * mj * inv * ry;
            }
            // Store accelerations to update everyone consistently later
            ax[i] = axi;
            ay[i] = ayi;
        }*/
        // Compute accelerations (symmetry / Newton's 3rd law)
        for (int i = 0; i < N; i++) {
            // Reset accumulated accelerations for particle i
            ax[i] = 0.0;
            ay[i] = 0.0;
        }
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
                double inv = 1.0 / (denom * denom * denom); // 1/(r+eps)^3
                // Update i and j using equal and opposite interaction
                double s_i = -G * mj * inv;
                double s_j =  G * mi * inv;

                ax[i] += s_i * rx;
                ay[i] += s_i * ry;

                ax[j] += s_j * rx;
                ay[j] += s_j * ry;
            }
        }
        // Update velocities and positions
        for (int i = 0; i < N; i++) {
        data[6*i + VX] += dt * ax[i];
        data[6*i + VY] += dt * ay[i];

        data[6*i + X]  += dt * data[6*i + VX];
        data[6*i + Y]  += dt * data[6*i + VY];
        }
    }
    //printf("Particle 0 after %d steps:\n", nsteps);
    //printf("  x=%g\n  y=%g\n  m=%g\n  vx=%g\n  vy=%g\n  b=%g\n",
    //   data[0], data[1], data[2], data[3], data[4], data[5]);
    // Clean up temporary arrays
    free(ax);
    free(ay);

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