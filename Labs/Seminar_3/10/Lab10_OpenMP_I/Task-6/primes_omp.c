#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

static int is_prime(long n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    long limit = (long)sqrt((double)n);
    for (long d = 3; d <= limit; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <M> <num_threads>\n", argv[0]);
        return 1;
    }

    long M = atol(argv[1]);
    int nthreads = atoi(argv[2]);
    if (M < 0 || nthreads <= 0) {
        printf("Error: M must be >= 0 and num_threads must be > 0\n");
        return 1;
    }

    printf("M=%ld, threads=%d\n", M, nthreads);

    // ---------- Serial ----------
    double t0 = omp_get_wtime();
    long count_serial = 0;
    for (long i = 1; i <= M; i++) {
        count_serial += is_prime(i);
    }
    double t1 = omp_get_wtime();
    printf("Serial: primes=%ld, time=%.6f s\n", count_serial, t1 - t0);

    // ---------- Parallel (bad load balance if static contiguous chunks) ----------
    double t2 = omp_get_wtime();
    long count_par_static = 0;

#pragma omp parallel for num_threads(nthreads) reduction(+:count_par_static) schedule(static)
    for (long i = 1; i <= M; i++) {
        count_par_static += is_prime(i);
    }

    double t3 = omp_get_wtime();
    printf("OMP static: primes=%ld, time=%.6f s\n", count_par_static, t3 - t2);

    // ---------- Parallel (better load balance) ----------
    double t4 = omp_get_wtime();
    long count_par_dynamic = 0;

#pragma omp parallel for num_threads(nthreads) reduction(+:count_par_dynamic) schedule(dynamic, 100)
    for (long i = 1; i <= M; i++) {
        count_par_dynamic += is_prime(i);
    }

    double t5 = omp_get_wtime();
    printf("OMP dynamic: primes=%ld, time=%.6f s\n", count_par_dynamic, t5 - t4);

    return 0;
}