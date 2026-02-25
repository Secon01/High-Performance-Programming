CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDLIBS  = -lm

TARGET  = galsim
SRC     = galsim_pthreads.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -pthread -o $@ $< $(LDLIBS)

# ---- fast build ----
fast: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast: clean all

clean:
	rm -f $(TARGET)

.PHONY: all clean fast