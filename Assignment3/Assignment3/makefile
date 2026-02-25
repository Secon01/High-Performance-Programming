CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDLIBS  = -lm

# default serial
TARGET  = galsim
SRC     = galsim.c
OBJ     = $(SRC:.c=.o)

# pthreads version
PTH_BIN = galsim_p
PTH_SRC = galsim_pthreads.c

# openmp version
OMP_BIN = galsim_omp
OMP_SRC = galsim_omp.c

# Apple Silicon Homebrew prefix (change to /usr/local on Intel Macs)
HOMEBREW_PREFIX = /opt/homebrew
OMP_INC = -I$(HOMEBREW_PREFIX)/opt/libomp/include
OMP_LIB = -L$(HOMEBREW_PREFIX)/opt/libomp/lib -lomp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- pthreads build ----
pthreads: $(PTH_BIN)

$(PTH_BIN): $(PTH_SRC)
	$(CC) $(CFLAGS) -pthread -o $@ $< $(LDLIBS)

# ---- openmp build (Apple clang compatible flags) ----
openmp: $(OMP_BIN)

$(OMP_BIN): $(OMP_SRC)
	$(CC) $(CFLAGS) -Xpreprocessor -fopenmp $(OMP_INC) -o $@ $< $(OMP_LIB) $(LDLIBS)

# ---- build everything ----
allvers: all pthreads openmp

# ---- fast builds ----
fast: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast: clean all

fast_pthreads: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast_pthreads: clean pthreads

fast_openmp: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast_openmp: clean openmp

clean:
	rm -f $(TARGET) $(OBJ) $(PTH_BIN) $(OMP_BIN)

.PHONY: all clean fast pthreads openmp allvers fast_pthreads fast_openmp