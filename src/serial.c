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
