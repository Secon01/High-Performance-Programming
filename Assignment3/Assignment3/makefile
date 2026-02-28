CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDLIBS  = -lm

TARGET  = galsim
PTH_BIN = galsim_p
OMP_BIN = galsim_omp

UNAME_S := $(shell uname -s)

# OpenMP flags: Linux (gcc) vs macOS (clang + libomp)
ifeq ($(UNAME_S),Darwin)
  HOMEBREW_PREFIX = /opt/homebrew
  OMP_FLAGS = -Xpreprocessor -fopenmp -I$(HOMEBREW_PREFIX)/opt/libomp/include
  OMP_LIBS  = -L$(HOMEBREW_PREFIX)/opt/libomp/lib -lomp
else
  OMP_FLAGS = -fopenmp
  OMP_LIBS  =
endif

all: $(TARGET)

$(TARGET): galsim.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

pthreads: $(PTH_BIN)

$(PTH_BIN): galsim_pthreads.c
	$(CC) $(CFLAGS) -pthread -o $@ $< $(LDLIBS)

openmp: $(OMP_BIN)

$(OMP_BIN): galsim_omp.c
	$(CC) $(CFLAGS) $(OMP_FLAGS) -o $@ $< $(OMP_LIBS) $(LDLIBS)

fast: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast: clean all

fast_pthreads: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast_pthreads: clean pthreads

fast_openmp: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast_openmp: clean openmp

clean:
	rm -f $(TARGET) $(PTH_BIN) $(OMP_BIN)

.PHONY: all pthreads openmp fast fast_pthreads fast_openmp clean