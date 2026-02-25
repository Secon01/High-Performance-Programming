CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDLIBS  = -lm

TARGET  = galsim
SRC     = galsim_omp.c

UNAME_S := $(shell uname -s)

# macOS (Apple clang) needs libomp
HOMEBREW_PREFIX ?= /opt/homebrew
OMP_INC = -I$(HOMEBREW_PREFIX)/opt/libomp/include
OMP_LIB = -L$(HOMEBREW_PREFIX)/opt/libomp/lib -lomp

ifeq ($(UNAME_S),Darwin)
  OMPFLAGS = -Xpreprocessor -fopenmp $(OMP_INC)
  OMPLIBS  = $(OMP_LIB)
else
  OMPFLAGS = -fopenmp
  OMPLIBS  =
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $< $(OMPLIBS) $(LDLIBS)

# ---- fast build ----
fast: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast: clean all

clean:
	rm -f $(TARGET)

.PHONY: all clean fast