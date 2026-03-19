CC     = gcc
CFLAGS = -std=c11 -O3 -Wall -Wextra -pthread
TARGET = gol
SRCS   = main.c gol.c
OBJS   = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

main.o: main.c gol.h

gol.o: gol.c gol.h

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
