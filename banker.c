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

int estado_seguro(void) {
    int work[m];        
    int finish[n];      
 
    for (int j = 0; j < m; j++) work[j] = available[j];
    for (int i = 0; i < n; i++) finish[i] = 0;
 
    int achou = 1;
    while (achou) {
        achou = 0;
        for (int i = 0; i < n; i++) {
            if (finish[i]) continue;
 
           
            int pode = 1;
            for (int j = 0; j < m; j++)
                if (need[i][j] > work[j]) { pode = 0; break; }
 
            if (pode) {
                
                for (int j = 0; j < m; j++) work[j] += allocation[i][j];
                finish[i] = 1;
                achou = 1;
            }
        }
    }
 
    
    for (int i = 0; i < n; i++)
        if (!finish[i]) return 0;
    return 1;
}

void requisicao(FILE *out, int c, int *req) {

    for (int j = 0; j < m; j++)
        if (req[j] > need[c][j]) {
            fprintf(out, "The customer %d request ", c);
            imprime_vetor(out, req);
            fprintf(out, "was denied because exceed its maximum allocation\n");
            return;
        }
 
    for (int j = 0; j < m; j++)
        if (req[j] > available[j]) {
            fprintf(out, "The resources ");
            imprime_vetor(out, available);
            fprintf(out, "are not enough to customer %d request ", c);
            imprime_vetor(out, req);
            fprintf(out, "\n");
            return;
        }
 
    for (int j = 0; j < m; j++) {
        available[j]     -= req[j];
        allocation[c][j] += req[j];
        need[c][j]       -= req[j];
    }
 
    if (!estado_seguro()) {
        for (int j = 0; j < m; j++) {
            available[j]     += req[j];
            allocation[c][j] -= req[j];
            need[c][j]       += req[j];
        }
        fprintf(out, "The customer %d request ", c);
        imprime_vetor(out, req);
        fprintf(out, "was denied because result in an unsafe state\n");
        return;
    }
 
    fprintf(out, "Allocate to customer %d the resources ", c);
    for (int j = 0; j < m; j++)
        fprintf(out, j ? " %d" : "%d", req[j]);   
    fprintf(out, "\n");
}

void le_clientes(void) {
    FILE *f = fopen("customer.txt", "r");
    if (!f) erro("nao foi possivel abrir customer.txt");
 
    
    char linha[1024];
    n = 0;
    while (fgets(linha, sizeof linha, f))
        if (strspn(linha, " \t\r\n") != strlen(linha)) n++;
    if (n == 0) erro("customer.txt vazio");
    rewind(f);
 
    maximum    = nova_matriz(n, m);
    allocation = nova_matriz(n, m);
    need       = nova_matriz(n, m);
 
    
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

int main(int argc, char *argv[]){             ////////////////////MAIN//////////////////////

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