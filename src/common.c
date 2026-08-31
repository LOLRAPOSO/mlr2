#include "common.h"
#include <string.h>
#include <errno.h>
#include <stdarg.h>

void log_error(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Erro: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void init_params(MandelbrotParams *params, int width, int height, int max_iter) {
    params->width    = width;
    params->height   = height;
    params->max_iter = max_iter;
    params->re_min   = RES_MIN;
    params->re_max   = RES_MAX;
    params->im_min   = IMG_MIN;
    params->im_max   = IMG_MAX;
}

int mandelbrot_point(double c_re, double c_im, int max_iter) {
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0; /* zr*zr e zi*zi, reaproveitados */
    int iter = 0;
 
    while (iter < max_iter && (zr2 + zi2) <= RAIO) {
        zi = 2.0 * zr * zi + c_im;
        zr = zr2 - zi2 + c_re;
        zr2 = zr * zr;
        zi2 = zi * zi;
        iter++;
    }
 
    return iter;
}

unsigned char normalize_iterations(int iter, int max_iter) {
    if (max_iter <= 0) {
        return 0;
    }
    double proporcao = (double) iter / (double) max_iter;
    int intensidade = (int) (proporcao * 255.0);
 
    if (intensidade < 0)   intensidade = 0;
    if (intensidade > 255) intensidade = 255;
 
    return (unsigned char) intensidade;
}

void compute_rows(const MandelbrotParams *params, unsigned char *pixels,
                   int row_start, int row_end) {
    int width  = params->width;
    int height = params->height;
 
    double re_range = params->re_max - params->re_min;
    double im_range = params->im_max - params->im_min;
 
    for (int y = row_start; y < row_end; y++) {
        /* mapeia a linha y (0..height-1) para a parte imaginária */
        double c_im = params->im_min + (y / (double) (height - 1)) * im_range;
 
        for (int x = 0; x < width; x++) {
            /* mapeia a coluna x (0..width-1) para a parte real */
            double c_re = params->re_min + (x / (double) (width - 1)) * re_range;
 
            int iter = mandelbrot_point(c_re, c_im, params->max_iter);
            pixels[y * width + x] = normalize_iterations(iter, params->max_iter);
        }
    }
}

int save_pgm_raw(const char *filename, const unsigned char *pixels,
                  int width, int height) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        log_error("nao foi possivel criar o arquivo '%s': %s",
                   filename, strerror(errno));
        return -1;
    }
 
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int valor = pixels[y * width + x];
 
            /* espaço entre valores, sem espaço sobrando no fim da linha */
            if (x > 0) {
                if (fputc(' ', f) == EOF) {
                    log_error("falha ao escrever no arquivo '%s'", filename);
                    fclose(f);
                    return -1;
                }
            }
            if (fprintf(f, "%d", valor) < 0) {
                log_error("falha ao escrever no arquivo '%s'", filename);
                fclose(f);
                return -1;
            }
        }
        if (fputc('\n', f) == EOF) {
            log_error("falha ao escrever no arquivo '%s'", filename);
            fclose(f);
            return -1;
        }
    }
 
    if (fclose(f) != 0) {
        log_error("falha ao fechar o arquivo '%s': %s", filename, strerror(errno));
        return -1;
    }
 
    return 0;
}

static int parse_positive_int(const char *str, const char *nome_campo,
                               long min_val, long max_val, int *out) {
    char *endptr;
    errno = 0;
 
    long valor = strtol(str, &endptr, 10);
 
    if (endptr == str || *endptr != '\0') {
        log_error("'%s' deve ser um numero inteiro valido (recebido: '%s')",
                   nome_campo, str);
        return -1;
    }
    if (errno == ERANGE) {
        log_error("'%s' esta fora da faixa representavel (recebido: '%s')",
                   nome_campo, str);
        return -1;
    }
    if (valor < min_val || valor > max_val) {
        log_error("'%s' deve estar entre %ld e %ld (recebido: %ld)",
                   nome_campo, min_val, max_val, valor);
        return -1;
    }
 
    *out = (int) valor;
    return 0;
}

int validate_args(int argc, char **argv, int *width, int *height,
                   int *max_iter, int *num_threads) {
    if (argc != 5) {
        log_error("numero de argumentos invalido.\n"
                   "Uso: %s [largura] [altura] [max_iteracoes] [num_threads]",
                   argc > 0 ? argv[0] : "mandelbrot");
        return -1;
    }
 
    if (parse_positive_int(argv[1], "largura", MIN_DIM, MAX_DIM, width) != 0)
        return -1;
 
    if (parse_positive_int(argv[2], "altura", MIN_DIM, MAX_DIM, height) != 0)
        return -1;
 
    if (parse_positive_int(argv[3], "max_iteracoes", MIN_MAX_ITER, MAX_MAX_ITER, max_iter) != 0)
        return -1;
 
    if (parse_positive_int(argv[4], "num_threads", MIN_THREADS, MAX_THREADS, num_threads) != 0)
        return -1;
 
    return 0;
}
