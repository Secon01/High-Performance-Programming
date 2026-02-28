CC      = gcc
CFLAGS  = -O3 -march=native -Wall -Wextra -std=c11
LDLIBS  = -lm
PTHREAD = -pthread

TARGET  = galsim
SRC     = galsim_pthreads.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(PTHREAD) -o $@ $< $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean