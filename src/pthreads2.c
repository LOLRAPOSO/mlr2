#define _POSIX_C_SOURCE 199309L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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
extern void log_error(const char *fmt, ...);

double run_pthreads2(const MandelbrotParams *params, unsigned char *pixels, int num_threads);

typedef struct {
    const MandelbrotParams *params;
    unsigned char *pixels;
    int thread_id;
    int num_threads;
} ThreadArgs2;


static void *worker_round_robin(void *arg) {
    ThreadArgs2 *targs = (ThreadArgs2 *) arg;
    const MandelbrotParams *params = targs->params;
 
    int width  = params->width;
    int height = params->height;
    double re_range = params->re_max - params->re_min;
    double im_range = params->im_max - params->im_min;
 
    for (int y = targs->thread_id; y < height; y += targs->num_threads) {
        double c_im = params->im_min + (y / (double) (height - 1)) * im_range;
 
        for (int x = 0; x < width; x++) {
            double c_re = params->re_min + (x / (double) (width - 1)) * re_range;
 
            int iter = mandelbrot_point(c_re, c_im, params->max_iter);
            targs->pixels[y * width + x] = normalize_iterations(iter, params->max_iter);
        }
    }
 
    return NULL;
}

double run_pthreads2(const MandelbrotParams *params, unsigned char *pixels,
                      int num_threads) {
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadArgs2 *args = malloc(num_threads * sizeof(ThreadArgs2));
 
    if (threads == NULL || args == NULL) {
        log_error("falha ao alocar memoria para threads (pthreads2)");
        free(threads);
        free(args);
        return -1.0;
    }
 
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
 
    int threads_criadas = 0;
    for (int t = 0; t < num_threads; t++) {
        args[t].params      = params;
        args[t].pixels      = pixels;
        args[t].thread_id   = t;
        args[t].num_threads = num_threads;
 
        int rc = pthread_create(&threads[t], NULL, worker_round_robin, &args[t]);
        if (rc != 0) {
            log_error("falha ao criar thread %d (pthreads2): codigo %d", t, rc);
            for (int j = 0; j < threads_criadas; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(args);
            return -1.0;
        }
        threads_criadas++;
    }
 
    for (int t = 0; t < threads_criadas; t++) {
        pthread_join(threads[t], NULL);
    }
 
    clock_gettime(CLOCK_MONOTONIC, &fim);
 
    free(threads);
    free(args);
 
    double segundos = (fim.tv_sec - inicio.tv_sec)
                     + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    return segundos;
}
