CC      = gcc
CFLAGS  = -O3 -march=native -Wall -Wextra -std=c11
LDLIBS  = -lm

TARGET  = galsim
SRC     = galsim_omp.c

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  # macOS: needs libomp (brew install libomp)
  OMPFLAGS = -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
  OMPLIBS  = -L/opt/homebrew/opt/libomp/lib -lomp
else
  # Linux gcc
  OMPFLAGS = -fopenmp
  OMPLIBS  =
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $< $(OMPLIBS) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean