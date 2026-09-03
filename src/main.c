#include "common.h"

#define LOGIN "mla"

extern double run_serial(const MandelbrotParams *params, unsigned char *pixels);
extern double run_openmp(const MandelbrotParams *params, unsigned char *pixels, int num_threads);
extern double run_pthreads1(const MandelbrotParams *params, unsigned char *pixels, int num_threads);
extern double run_pthreads2(const MandelbrotParams *params, unsigned char *pixels, int num_threads);

static int run_and_save(FILE *times_file, const char *nome_impl, double tempo, const unsigned char *pixels, int width, int height, const char *pgm_filename) {
    
    if (tempo < 0.0) {
        log_error("implementacao '%s' falhou, abortando", nome_impl);
        return -1;
    }
 
    if (save_pgm_raw(pgm_filename, pixels, width, height) != 0) {
        return -1;
    }
 
    if (fprintf(times_file, "%s: %.6f segundos\n", nome_impl, tempo) < 0) {
        log_error("falha ao escrever tempo de '%s' em times.txt", nome_impl);
        return -1;
    }
 
    return 0;
}

int main(int argc, char *argv[])
{
    int width, height, max_iter, num_threads;

    if (validate_args(argc, argv, &width, &height, &max_iter, &num_threads) != 0) {
        return EXIT_FAILURE;
    }
 
    MandelbrotParams params;
    init_params(&params, width, height, max_iter);

    unsigned char *pixels = malloc((size_t) width * (size_t) height);
    if (pixels == NULL) {
        log_error("falha ao alocar memoria para o buffer de pixels "
                   "(%dx%d)", width, height);
        return EXIT_FAILURE;
    }
 
    FILE *times_file = fopen("times.txt", "w");
    if (times_file == NULL) {
        log_error("nao foi possivel criar o arquivo 'times.txt'");
        free(pixels);
        return EXIT_FAILURE;
    }
 
    int status = EXIT_SUCCESS;
    double tempo;
    char filename[256];
 
    tempo = run_serial(&params, pixels);
    snprintf(filename, sizeof(filename), "mandelbrot_%s_serial.pgm", LOGIN);
    if (run_and_save(times_file, "serial", tempo, pixels, width, height, filename) != 0) {
        status = EXIT_FAILURE;
        goto cleanup;
    }

    tempo = run_openmp(&params, pixels, num_threads);
    snprintf(filename, sizeof(filename), "mandelbrot_%s_openmp.pgm", LOGIN);
    if (run_and_save(times_file, "openmp", tempo, pixels, width, height, filename) != 0) {
        status = EXIT_FAILURE;
        goto cleanup;
    }
 
    tempo = run_pthreads1(&params, pixels, num_threads);
    snprintf(filename, sizeof(filename), "mandelbrot_%s_pthreads1.pgm", LOGIN);
    if (run_and_save(times_file, "pthreads1", tempo, pixels, width, height, filename) != 0) {
        status = EXIT_FAILURE;
        goto cleanup;
    }
 
    tempo = run_pthreads2(&params, pixels, num_threads);
    snprintf(filename, sizeof(filename), "mandelbrot_%s_pthreads2.pgm", LOGIN);
    if (run_and_save(times_file, "pthreads2", tempo, pixels, width, height, filename) != 0) {
        status = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    fclose(times_file);
    free(pixels);
    return status;
}