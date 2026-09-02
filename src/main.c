#include "common.h"

#define LOGIN "mlr2"

typedef struct {
    int width;
    int height;
    int max_iter;
    double re_min;
    double re_max;
    double im_min;
    double im_max;
} MandelbrotParams;

extern double run_serial(const MandelbrotParams *params, unsigned char *pixels);
extern double run_openmp(const MandelbrotParams *params, unsigned char *pixels, int num_threads);
extern double run_pthreads1(const MandelbrotParams *params, unsigned char *pixels, int num_threads);
extern double run_pthreads2(const MandelbrotParams *params, unsigned char *pixels, int num_threads);

int main(int argc, char *argv[])
{
     
}