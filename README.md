# High Performance Programming

Coursework for the High Performance Programming course, M.Sc. Data Science,
Uppsala University. Written in C, focused on profiling, optimisation and
shared-memory parallelism.

## N-body galaxy simulator — `Assignment3/`, `Assignment4/`

A gravitational N-body simulation implemented three ways: serial, OpenMP and
pthreads.

The serial version uses direct O(N^2) summation with Newton's third law to halve
the force computations. Profiling with gprof showed force computation accounts
for over 99% of runtime at N >= 5,000, so parallelisation targets that loop.

Measured on an Apple M4 Pro, 100 timesteps, dt = 1e-5, clang -O3 -march=native:

| Implementation | N | 1 thread | 8 threads | Speedup |
|---|---|---|---|---|
| OpenMP | 10,000 | 12.460 s | 2.790 s | 4.5x |
| OpenMP | 5,000 | 3.327 s | 0.762 s | 4.4x |
| OpenMP | 2,000 | 0.395 s | 0.147 s | 2.7x |
| pthreads | 10,000 | 12.404 s | 2.742 s | 4.5x |
| pthreads | 5,000 | 3.287 s | 0.731 s | 4.5x |

Scaling across 1/2/4/8 threads at N=10,000: 1.0x / 1.3x / 2.4x / 4.4x.

Speedup falls to 2.7x at N=2,000 because the serial update phase grows from 0.1%
to 4.6% of runtime, so parallel overhead is no longer amortised.

Raw measurements: `Assignment3/Assignment3/timing_*.txt` and `scaling_openmp_p*.txt`.

## Game of Life — `Project/`

A parallel Game of Life using pthreads with mutexes and condition variables for
phase synchronisation between generations.

Correctness was verified by confirming that 1-thread and 4-thread outputs are
byte-identical on both small and large grids — the diff files in
`Project/gol/report_data/` are empty. The build was also run under
AddressSanitizer, and three edge cases are documented.

On a 2000x2000 grid over 200 steps: about 0.203 s single-threaded, about 0.035 s
at 8 threads. Performance plateaus beyond 8 threads as the stencil becomes
memory-bandwidth bound.

## Other contents

| Folder | Contents |
|---|---|
| `Assignment1/`, `Assignment2/` | C fundamentals, Makefiles, file I/O, Pascal's triangle |
| `Labs/` | 11 lab sessions: Linux tooling, debugging, memory usage, instruction-level parallelism, pthreads, OpenMP |
| `Exam/` | Exam preparation material |

## Build

Each assignment and lab has its own Makefile.

    cd Assignment3/Assignment3
    make
    ./galsim <N> <input_file> <nsteps> <dt> <graphics>
