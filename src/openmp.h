#ifndef OPENMP_H
#define OPENMP_H
 
#include "common.h"

double run_openmp(const MandelbrotParams *params, unsigned char *pixels, int num_threads);
#endif 
