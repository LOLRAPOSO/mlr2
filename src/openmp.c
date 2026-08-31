#include <omp.h>

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

double run_openmp(const MandelbrotParams *params, unsigned char *pixels, int num_threads);

double run_openmp(const MandelbrotParams *params, unsigned char *pixels,
                   int num_threads) {
    int width  = params->width;
    int height = params->height;
 
    double re_range = params->re_max - params->re_min;
    double im_range = params->im_max - params->im_min;
 
    double inicio = omp_get_wtime();
 
    #pragma omp parallel for num_threads(num_threads) schedule(dynamic) \
            default(none) shared(params, pixels, width, height, re_range, im_range)
    for (int y = 0; y < height; y++) {
        double c_im = params->im_min + (y / (double) (height - 1)) * im_range;
 
        for (int x = 0; x < width; x++) {
            double c_re = params->re_min + (x / (double) (width - 1)) * re_range;
 
            int iter = mandelbrot_point(c_re, c_im, params->max_iter);
            pixels[y * width + x] = normalize_iterations(iter, params->max_iter);
        }
    }
 
    double fim = omp_get_wtime();
    return fim - inicio;
}
