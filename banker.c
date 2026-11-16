#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int n;
int m;
int *available;
int **maximum;
int **allocation;
int **need;

void erro(const char *msg) {
    fprintf(stderr, "Erro: %s\n", msg);
    exit(1);
}

int **nova_matriz(int linhas, int colunas) {
    int **a = malloc(linhas * sizeof(int *));
    for (int i = 0; i < linhas; i++)
        a[i] = calloc(colunas, sizeof(int));
    return a;
}

void imprime_vetor(FILE *f, int *v) {
    for (int j = 0; j < m; j++)
        fprintf(f, "%d ", v[j]);
}

void imprime_vetor(FILE *f, int *v) {
    for (int j = 0; j < m; j++)
        fprintf(f, "%d ", v[j]);
}

int estado_seguro(void) {
    int work[m];        /* copia de available */
    int finish[n];      /* finish[i] = 1 se o cliente i "terminou" */
 
    for (int j = 0; j < m; j++) work[j] = available[j];
    for (int i = 0; i < n; i++) finish[i] = 0;
 
    int achou = 1;
    while (achou) {
        achou = 0;
        for (int i = 0; i < n; i++) {
            if (finish[i]) continue;
 
            /* o cliente i consegue terminar com o que esta em work? */
            int pode = 1;
            for (int j = 0; j < m; j++)
                if (need[i][j] > work[j]) { pode = 0; break; }
 
            if (pode) {
                /* ele termina e devolve tudo que tinha */
                for (int j = 0; j < m; j++) work[j] += allocation[i][j];
                finish[i] = 1;
                achou = 1;
            }
        }
    }
 
    /* seguro somente se TODOS terminaram */
    for (int i = 0; i < n; i++)
        if (!finish[i]) return 0;
    return 1;
}

void le_clientes(void) {
    FILE *f = fopen("customer.txt", "r");
    if (!f) erro("nao foi possivel abrir customer.txt");
 
    /* 1a passada: conta as linhas nao vazias = numero de clientes */
    char linha[1024];
    n = 0;
    while (fgets(linha, sizeof linha, f))
        if (strspn(linha, " \t\r\n") != strlen(linha)) n++;
    if (n == 0) erro("customer.txt vazio");
    rewind(f);
 
    maximum    = nova_matriz(n, m);
    allocation = nova_matriz(n, m);
    need       = nova_matriz(n, m);
 
    /* 2a passada: le os valores separados por virgula */
    int i = 0;
    while (fgets(linha, sizeof linha, f)) {
        if (strspn(linha, " \t\r\n") == strlen(linha)) continue;
 
        int cont = 0;
        for (char *tok = strtok(linha, ",\r\n"); tok; tok = strtok(NULL, ",\r\n")) {
            char *fim;
            long v = strtol(tok, &fim, 10);
            while (*fim == ' ' || *fim == '\t') fim++;
            if (fim == tok || *fim != '\0' || v < 0)
                erro("customer.txt com valor invalido");
            if (cont < m) maximum[i][cont] = (int)v;
            cont++;
        }
        if (cont != m)
            erro("numero de colunas do customer.txt difere do numero de recursos");
 
        for (int j = 0; j < m; j++) need[i][j] = maximum[i][j];
        i++;
    }
    fclose(f);
}

void imprime_estado(FILE *out) {
    fprintf(out, "MAXIMUM   |   ALLOCATION  |   NEED\n");
    for (int i = 0; i < n; i++) {
        imprime_vetor(out, maximum[i]);    fprintf(out, "    |   ");
        imprime_vetor(out, allocation[i]); fprintf(out, "      |   ");
        imprime_vetor(out, need[i]);       fprintf(out, "\n");
    }
    fprintf(out, "AVAILABLE ");
    imprime_vetor(out, available);
    fprintf(out, "\n");
}

int main(int argc, char *argv[]){

    if (argc < 2){ 
        erro("uso: ./banker <inst_recurso0> <inst_recurso1> ...");
    }

    m = argc - 1;
    available = malloc(m * sizeof(int));
    for (int j = 0; j < m; j++) {
        char *fim;
        long v = strtol(argv[j + 1], &fim, 10);
        if (*fim != '\0' || v < 0) erro("argumento invalido");
        available[j] = (int)v;
    }
    le_clientes();

    FILE *cmd = fopen("commands.txt", "r");
    if (!cmd) erro("nao foi possivel abrir commands.txt");
    FILE *out = fopen("result.txt", "w");
    if (!out) erro("nao foi possivel criar result.txt");
}