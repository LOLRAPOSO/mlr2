#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int n;
int m;
int *available;
int **maximum;
int **allocation;
int **need;

int **nova_matriz(int linhas, int colunas) {
    int **a = malloc(linhas * sizeof(int *));
    for (int i = 0; i < linhas; i++)
        a[i] = calloc(colunas, sizeof(int));
    return a;
}

int main(int argc, char *argv[]){

}