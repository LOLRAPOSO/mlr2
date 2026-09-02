#define _POSIX_C_SOURCE 199309L
#include <time.h>

typedef struct {
    int width;
    int height;
    int max_iter;
    double re_min;
    double re_max;
    double im_min;
    double im_max;
} MandelbrotParams;

extern int mandelbrot_point(double c_re, double c_im, int max_iter);
extern unsigned char normalize_iterations(int iter, int max_iter);

double run_serial(const MandelbrotParams *params, unsigned char *pixels);

double run_serial(const MandelbrotParams *params, unsigned char *pixels) {
    int width  = params->width;
    int height = params->height;
 
    double re_range = params->re_max - params->re_min;
    double im_range = params->im_max - params->im_min;
 
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
 
    for (int y = 0; y < height; y++) {
        double c_im = params->im_min + (y / (double) (height - 1)) * im_range;
 
        for (int x = 0; x < width; x++) {
            double c_re = params->re_min + (x / (double) (width - 1)) * re_range;
 
            int iter = mandelbrot_point(c_re, c_im, params->max_iter);
            pixels[y * width + x] = normalize_iterations(iter, params->max_iter);
        }
    }
 
    clock_gettime(CLOCK_MONOTONIC, &fim);
 
    double segundos = (fim.tv_sec - inicio.tv_sec)
                     + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    return segundos;
}
