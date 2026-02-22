CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -g

TARGET  = gol
SRCS    = main.c gol.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

main.o: main.c gol.h
	$(CC) $(CFLAGS) -c main.c -o main.o

gol.o: gol.c gol.h
	$(CC) $(CFLAGS) -c gol.c -o gol.o

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean