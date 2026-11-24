/*
 * Implementacao 6 - Infraestrutura de Software (CESAR School)
 * Designing a Virtual Memory Manager (Silberschatz, Operating System Concepts, 10a ed.)
 *
 * Uso: ./vm <arquivo_enderecos> <fifo|lru> <fifo|lru>
 *        arg 1: arquivo com enderecos logicos (um inteiro por linha)
 *        arg 2: algoritmo de substituicao de PAGINA (memoria fisica)
 *        arg 3: algoritmo de substituicao da TLB
 *
 * Saida: correct.txt  ("Virtual address: X Physical address: Y Value: Z")
 *
 * Parametros de compilacao (usados apenas para gerar as variantes de teste
 * pedidas na rubrica; o "make" padrao gera o programa com 128 frames + TLB
 * com threads):
 *   -DFRAMES=N        numero de frames da memoria fisica (padrao 128)
 *   -DUSE_TLB=0       desliga a TLB (padrao 1)
 *   -DTLB_THREADS=0   busca na TLB sequencial, sem threads (padrao 1)
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifndef FRAMES
#define FRAMES 128
#endif
#ifndef USE_TLB
#define USE_TLB 1
#endif
#ifndef TLB_THREADS
#define TLB_THREADS 1
#endif

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define TLB_SIZE 16
#define STORE_FILE "BACKING_STORE.bin"
#define STORE_BYTES (NUM_PAGES * PAGE_SIZE)
#define OUT_FILE "correct.txt"

typedef enum { ALG_FIFO, ALG_LRU } alg_t;

/* ---------------- estado da memoria ---------------- */
static signed char physmem[FRAMES][PAGE_SIZE];
static int page_table[NUM_PAGES];        /* pagina -> frame (-1 = nao residente) */
static int frame_page[FRAMES];           /* frame  -> pagina residente           */
static unsigned long frame_used[FRAMES]; /* instante do ultimo acesso (LRU)      */
static int frames_in_use = 0;
static int fifo_next = 0;                /* proximo frame a ser substituido (FIFO) */
static unsigned long tick = 0;           /* relogio logico                       */

/* ---------------- TLB ---------------- */
typedef struct {
    int valid;
    int page;
    int frame;
    unsigned long loaded_at;  /* FIFO */
    unsigned long last_used;  /* LRU  */
} tlb_entry;
static tlb_entry tlb[TLB_SIZE];

static alg_t page_alg, tlb_alg;
static FILE *store;

/* estatisticas */
static long n_addr = 0, n_faults = 0, n_page_evictions = 0;
static long n_tlb_hits __attribute__((unused)) = 0;

/* ---------------- busca na TLB ---------------- */
typedef struct {
    int idx;
    int page;
    int hit;
} tlb_job;

/* cada thread inspeciona UMA entrada da TLB e grava apenas no proprio job */
static void *tlb_worker(void *arg) {
    tlb_job *j = (tlb_job *)arg;
    const tlb_entry *e = &tlb[j->idx];
    j->hit = (e->valid && e->page == j->page);
    return NULL;
}

/* retorna indice da entrada que contem a pagina, ou -1 */
__attribute__((unused)) static int tlb_lookup(int page) {
    tlb_job jobs[TLB_SIZE];
    for (int i = 0; i < TLB_SIZE; i++) {
        jobs[i].idx = i;
        jobs[i].page = page;
        jobs[i].hit = 0;
    }
#if TLB_THREADS
    pthread_t th[TLB_SIZE];
    int started[TLB_SIZE];
    for (int i = 0; i < TLB_SIZE; i++) {
        started[i] = (pthread_create(&th[i], NULL, tlb_worker, &jobs[i]) == 0);
        if (!started[i]) tlb_worker(&jobs[i]); /* fallback se nao der para criar thread */
    }
    for (int i = 0; i < TLB_SIZE; i++)
        if (started[i]) pthread_join(th[i], NULL);
#else
    for (int i = 0; i < TLB_SIZE; i++) tlb_worker(&jobs[i]);
#endif
    for (int i = 0; i < TLB_SIZE; i++)
        if (jobs[i].hit) return i;
    return -1;
}

__attribute__((unused)) static void tlb_insert(int page, int frame) {
    int victim = -1;
    for (int i = 0; i < TLB_SIZE; i++)
        if (!tlb[i].valid) { victim = i; break; }
    if (victim < 0) {
        victim = 0;
        for (int i = 1; i < TLB_SIZE; i++) {
            if (tlb_alg == ALG_FIFO ? tlb[i].loaded_at < tlb[victim].loaded_at
                                    : tlb[i].last_used < tlb[victim].last_used)
                victim = i;
        }
    }
    tlb[victim].valid = 1;
    tlb[victim].page = page;
    tlb[victim].frame = frame;
    tlb[victim].loaded_at = tlb[victim].last_used = ++tick;
}

static void tlb_invalidate_page(int page) {
    for (int i = 0; i < TLB_SIZE; i++)
        if (tlb[i].valid && tlb[i].page == page) tlb[i].valid = 0;
}

/* ---------------- memoria fisica / page fault ---------------- */
static int choose_victim_frame(void) {
    if (page_alg == ALG_FIFO) {
        int v = fifo_next;
        fifo_next = (fifo_next + 1) % FRAMES;
        return v;
    }
    int v = 0;
    for (int f = 1; f < FRAMES; f++)
        if (frame_used[f] < frame_used[v]) v = f;
    return v;
}

/* trata page fault: devolve o frame onde a pagina foi carregada, ou -1 em erro de E/S */
static int handle_page_fault(int page) {
    int frame;
    if (frames_in_use < FRAMES) {
        frame = frames_in_use++;           /* enche de 0 a FRAMES-1 */
    } else {
        frame = choose_victim_frame();
        int old = frame_page[frame];
        page_table[old] = -1;
        tlb_invalidate_page(old);          /* evita traducao obsoleta na TLB */
        n_page_evictions++;
    }
    if (fseek(store, (long)page * PAGE_SIZE, SEEK_SET) != 0 ||
        fread(physmem[frame], 1, PAGE_SIZE, store) != PAGE_SIZE) {
        fprintf(stderr, "Erro: falha ao ler a pagina %d de %s\n", page, STORE_FILE);
        return -1;
    }
    page_table[page] = frame;
    frame_page[frame] = page;
    return frame;
}

/* ---------------- validacao de entrada ---------------- */
static int parse_alg(const char *s, alg_t *out) {
    if (strcasecmp(s, "fifo") == 0) { *out = ALG_FIFO; return 0; }
    if (strcasecmp(s, "lru") == 0)  { *out = ALG_LRU;  return 0; }
    return -1;
}

static void usage(const char *prog) {
    fprintf(stderr, "Uso: %s <arquivo_de_enderecos> <fifo|lru> <fifo|lru>\n"
                    "  arg 2: algoritmo de substituicao de pagina\n"
                    "  arg 3: algoritmo de substituicao da TLB\n"
                    "Exemplo: %s address.txt lru fifo\n", prog, prog);
}

/*
 * Le e valida TODO o arquivo antes de processar qualquer endereco.
 * Formato aceito: um inteiro decimal nao negativo (<= 4294967295) por linha;
 * linhas em branco so sao aceitas no final do arquivo.
 */
static int load_addresses(const char *path, uint32_t **out, size_t *count) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo de enderecos '%s': %s\n",
                path, strerror(errno));
        return -1;
    }
    size_t cap = 1024, n = 0;
    uint32_t *v = malloc(cap * sizeof *v);
    char *line = NULL;
    size_t lcap = 0;
    ssize_t len;
    long lineno = 0;
    int seen_blank = 0;
    if (!v) { fprintf(stderr, "Erro: memoria insuficiente\n"); fclose(f); return -1; }

    while ((len = getline(&line, &lcap, f)) != -1) {
        lineno++;
        if (memchr(line, '\0', (size_t)len)) {
            fprintf(stderr, "Erro: formato invalido em '%s' (linha %ld contem byte nulo)\n", path, lineno);
            goto fail;
        }
        while (len > 0 && isspace((unsigned char)line[len - 1])) line[--len] = '\0';
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') { seen_blank = 1; continue; }
        if (seen_blank) {
            fprintf(stderr, "Erro: formato invalido em '%s' (linha em branco antes da linha %ld)\n", path, lineno);
            goto fail;
        }
        for (char *q = p; *q; q++) {
            if (!isdigit((unsigned char)*q)) {
                fprintf(stderr, "Erro: formato invalido em '%s' (linha %ld: \"%s\" nao e um inteiro nao negativo)\n",
                        path, lineno, p);
                goto fail;
            }
        }
        errno = 0;
        unsigned long long val = strtoull(p, NULL, 10);
        if (errno == ERANGE || val > 0xFFFFFFFFULL) {
            fprintf(stderr, "Erro: formato invalido em '%s' (linha %ld: valor fora do intervalo de 32 bits)\n",
                    path, lineno);
            goto fail;
        }
        if (n == cap) {
            cap *= 2;
            uint32_t *nv = realloc(v, cap * sizeof *v);
            if (!nv) { fprintf(stderr, "Erro: memoria insuficiente\n"); goto fail; }
            v = nv;
        }
        v[n++] = (uint32_t)val;
    }
    if (ferror(f)) {
        fprintf(stderr, "Erro: falha ao ler '%s'\n", path);
        goto fail;
    }
    if (n == 0) {
        fprintf(stderr, "Erro: o arquivo '%s' esta vazio (nenhum endereco encontrado)\n", path);
        goto fail;
    }
    free(line);
    fclose(f);
    *out = v;
    *count = n;
    return 0;
fail:
    free(line);
    free(v);
    fclose(f);
    return -1;
}

static int open_store(void) {
    store = fopen(STORE_FILE, "rb");
    if (!store) {
        fprintf(stderr, "Erro: nao foi possivel abrir %s: %s\n", STORE_FILE, strerror(errno));
        return -1;
    }
    if (fseek(store, 0, SEEK_END) != 0) { fprintf(stderr, "Erro: falha ao inspecionar %s\n", STORE_FILE); return -1; }
    long sz = ftell(store);
    if (sz != STORE_BYTES) {
        fprintf(stderr, "Erro: %s deve ter %d bytes (tem %ld)\n", STORE_FILE, STORE_BYTES, sz);
        return -1;
    }
    return 0;
}

/* ---------------- main ---------------- */
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Erro: numero de argumentos invalido (esperado 3, recebido %d)\n", argc - 1);
        usage(argv[0]);
        return 1;
    }
    if (parse_alg(argv[2], &page_alg) != 0) {
        fprintf(stderr, "Erro: algoritmo de pagina invalido '%s' (use fifo ou lru)\n", argv[2]);
        usage(argv[0]);
        return 1;
    }
    if (parse_alg(argv[3], &tlb_alg) != 0) {
        fprintf(stderr, "Erro: algoritmo de TLB invalido '%s' (use fifo ou lru)\n", argv[3]);
        usage(argv[0]);
        return 1;
    }

    uint32_t *addrs = NULL;
    size_t total = 0;
    if (load_addresses(argv[1], &addrs, &total) != 0) return 1;
    if (open_store() != 0) { free(addrs); return 1; }

    FILE *out = fopen(OUT_FILE, "w");
    if (!out) {
        fprintf(stderr, "Erro: nao foi possivel criar %s: %s\n", OUT_FILE, strerror(errno));
        free(addrs); fclose(store);
        return 1;
    }

    for (int p = 0; p < NUM_PAGES; p++) page_table[p] = -1;

    for (size_t i = 0; i < total; i++) {
        uint32_t laddr = addrs[i] & 0xFFFF;       /* 16 bits */
        int page = (int)(laddr >> 8);
        int offset = (int)(laddr & 0xFF);
        int frame = -1;
        n_addr++;

#if USE_TLB
        int ti = tlb_lookup(page);
        if (ti >= 0) {
            frame = tlb[ti].frame;
            tlb[ti].last_used = ++tick;
            n_tlb_hits++;
        }
#endif
        if (frame < 0) {
            frame = page_table[page];
            if (frame < 0) {
                n_faults++;
                frame = handle_page_fault(page);
                if (frame < 0) { fclose(out); fclose(store); free(addrs); return 1; }
            }
#if USE_TLB
            tlb_insert(page, frame);
#endif
        }
        frame_used[frame] = ++tick;

        int phys = frame * PAGE_SIZE + offset;
        int value = physmem[frame][offset];
        fprintf(out, "Virtual address: %u Physical address: %d Value: %d\n", addrs[i], phys, value);
    }

    if (fclose(out) != 0) fprintf(stderr, "Aviso: erro ao fechar %s\n", OUT_FILE);
    fclose(store);
    free(addrs);

    printf("Configuracao: frames=%d, TLB=%s, threads=%s, pagina=%s, tlb_alg=%s\n",
           FRAMES, USE_TLB ? "sim" : "nao", (USE_TLB && TLB_THREADS) ? "sim" : "nao",
           page_alg == ALG_FIFO ? "fifo" : "lru", tlb_alg == ALG_FIFO ? "fifo" : "lru");
    printf("Number of Translated Addresses = %ld\n", n_addr);
    printf("Page Faults = %ld\n", n_faults);
    printf("Page Fault Rate = %.3f\n", (double)n_faults / n_addr);
    printf("Page Replacements = %ld\n", n_page_evictions);
#if USE_TLB
    printf("TLB Hits = %ld\n", n_tlb_hits);
    printf("TLB Hit Rate = %.3f\n", (double)n_tlb_hits / n_addr);
#endif
    printf("Saida gravada em %s\n", OUT_FILE);
    return 0;
}
