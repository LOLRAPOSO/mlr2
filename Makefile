CC = gcc
CFLAGS = -Wall -O2 -fopenmp
LDFLAGS = -lpthread -lm

all: mandelbrot

mandelbrot: src/main.o src/mandelbrot_common.o src/mandelbrot_serial.o src/mandelbrot_openmp.o src/mandelbrot_pthreads1.o src/mandelbrot_pthreads2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o mandelbrot *.pgm times.txt