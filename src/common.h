#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RES_MIN -2.0
#define RES_MAX  1.0
#define IMG_MIN -1.5
#define IMG_MAX  1.5
#define MIN_DIM 1
#define MAX_DIM 20000
#define MIN_MAX_ITER 1
#define MAX_MAX_ITER 1000000
#define MIN_THREADS 1
#define MAX_THREADS 256
#define RAIO  4.0

typedef struct {
    int width;
    int height;
    int max_iter;
    double re_min;
    double re_max;
    double im_min;
    double im_max;
} MandelbrotParams;

void init_params(MandelbrotParams *params, int width, int height, int max_iter);
int mandelbrot_point(double c_re, double c_im, int max_iter);
void compute_rows(const MandelbrotParams *params, unsigned char *pixels, int row_start, int row_end);
int save_pgm_raw(const char *filename, const unsigned char *pixels, int width, int height);
int validate_args(int argc, char **argv, int *width, int *height, int *max_iter, int *num_threads);
void log_error(const char *fmt, ...);

#endif /* COMMON_H */

