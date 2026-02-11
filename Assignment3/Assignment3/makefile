CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lm

TARGET  = galsim
SRC     = galsim.c
OBJ     = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

fast: CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
fast: clean all

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all fast clean
