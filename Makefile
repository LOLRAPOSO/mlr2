CC = gcc
CFLAGS = -Wall -O2 -fopenmp 
LDFLAGS = -lpthread -lm

all: mandelbrot

mandelbrot: src/main.o src/common.o src/serial.o src/openmp.o src/pthreads1.o src/pthreads2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o mandelbrot *.pgm times.txt