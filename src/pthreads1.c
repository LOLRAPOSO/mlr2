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

double run_pthreads1(const MandelbrotParams *params, unsigned char *pixels, int num_threads);

typedef struct {
    const MandelbrotParams *params;
    unsigned char *pixels;
    int row_start;
    int row_end;
} ThreadArgs1;

static void *worker_static(void *arg) {
    ThreadArgs1 *targs = (ThreadArgs1 *) arg;
    const MandelbrotParams *params = targs->params;
 
    int width  = params->width;
    int height = params->height;
    double re_range = params->re_max - params->re_min;
    double im_range = params->im_max - params->im_min;
 
    for (int y = targs->row_start; y < targs->row_end; y++) {
        double c_im = params->im_min + (y / (double) (height - 1)) * im_range;
 
        for (int x = 0; x < width; x++) {
            double c_re = params->re_min + (x / (double) (width - 1)) * re_range;
 
            int iter = mandelbrot_point(c_re, c_im, params->max_iter);
            targs->pixels[y * width + x] = normalize_iterations(iter, params->max_iter);
        }
    }
 
    return NULL;
}

double run_pthreads1(const MandelbrotParams *params, unsigned char *pixels,
                      int num_threads) {
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadArgs1 *args = malloc(num_threads * sizeof(ThreadArgs1));
 
    if (threads == NULL || args == NULL) {
        log_error("falha ao alocar memoria para threads (pthreads1)");
        free(threads);
        free(args);
        return -1.0;
    }
 
    int height = params->height;
    int linhas_por_thread = height / num_threads;
    int resto = height % num_threads;
 
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
 
    int linha_atual = 0;
    int threads_criadas = 0;
 
    for (int t = 0; t < num_threads; t++) {
        int bloco = linhas_por_thread + (t < resto ? 1 : 0);
 
        args[t].params    = params;
        args[t].pixels    = pixels;
        args[t].row_start = linha_atual;
        args[t].row_end   = linha_atual + bloco;
        linha_atual += bloco;
 
        int rc = pthread_create(&threads[t], NULL, worker_static, &args[t]);
        if (rc != 0) {
            log_error("falha ao criar thread %d (pthreads1): codigo %d", t, rc);
            /* junta as threads que já foram criadas antes de desistir,
             * para não deixá-las soltas (evita comportamento indefinido) */
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
