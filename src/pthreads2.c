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
    int next_row;            /* protegido por 'mutex' */
    pthread_mutex_t mutex;
} SharedWork;

static void *worker_dynamic(void *arg) {
    SharedWork *work = (SharedWork *) arg;
    const MandelbrotParams *params = work->params;
 
    int width  = params->width;
    int height = params->height;
    double re_range = params->re_max - params->re_min;
    double im_range = params->im_max - params->im_min;
 
    while (1) {
        pthread_mutex_lock(&work->mutex);
        int y = work->next_row;
        if (y < height) {
            work->next_row++;
        }
        pthread_mutex_unlock(&work->mutex);
 
        if (y >= height) {
            break; /* não sobraram mais linhas: esta thread termina */
        }
 
        double c_im = params->im_min + (y / (double) (height - 1)) * im_range;
 
        for (int x = 0; x < width; x++) {
            double c_re = params->re_min + (x / (double) (width - 1)) * re_range;
 
            int iter = mandelbrot_point(c_re, c_im, params->max_iter);
            work->pixels[y * width + x] = normalize_iterations(iter, params->max_iter);
        }
    }
 
    return NULL;
}

double run_pthreads2(const MandelbrotParams *params, unsigned char *pixels,
                      int num_threads) {
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    if (threads == NULL) {
        log_error("falha ao alocar memoria para threads (pthreads2)");
        return -1.0;
    }
 
    SharedWork work;
    work.params   = params;
    work.pixels   = pixels;
    work.next_row = 0;
 
    if (pthread_mutex_init(&work.mutex, NULL) != 0) {
        log_error("falha ao inicializar mutex (pthreads2)");
        free(threads);
        return -1.0;
    }
 
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
 
    int threads_criadas = 0;
    for (int t = 0; t < num_threads; t++) {
        int rc = pthread_create(&threads[t], NULL, worker_dynamic, &work);
        if (rc != 0) {
            log_error("falha ao criar thread %d (pthreads2): codigo %d", t, rc);
            for (int j = 0; j < threads_criadas; j++) {
                pthread_join(threads[j], NULL);
            }
            pthread_mutex_destroy(&work.mutex);
            free(threads);
            return -1.0;
        }
        threads_criadas++;
    }
 
    for (int t = 0; t < threads_criadas; t++) {
        pthread_join(threads[t], NULL);
    }
 
    clock_gettime(CLOCK_MONOTONIC, &fim);
 
    pthread_mutex_destroy(&work.mutex);
    free(threads);
 
    double segundos = (fim.tv_sec - inicio.tv_sec)
                     + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    return segundos;
}