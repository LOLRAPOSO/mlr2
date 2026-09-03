# Mandelbrot — mlr2

Implementação do conjunto de **Mandelbrot em C**, desenvolvida para estudar e comparar diferentes estratégias de execução **serial e paralela** utilizando **OpenMP** e **Pthreads**.

O programa gera a mesma imagem do conjunto de Mandelbrot utilizando quatro implementações diferentes e registra o tempo de execução de cada uma, permitindo analisar o ganho de desempenho obtido através da paralelização.

## 📌 Funcionalidades

O projeto possui quatro versões para o cálculo do conjunto de Mandelbrot:

* **Serial** — execução sequencial, sem paralelização.
* **OpenMP** — paralelização utilizando diretivas OpenMP.
* **Pthreads 1** — paralelização utilizando Pthreads com divisão estática das linhas da imagem.
* **Pthreads 2** — paralelização utilizando Pthreads com distribuição das linhas em esquema *round-robin*.

Todas as implementações produzem a mesma imagem, permitindo comparar seus tempos de execução.

## 🗂️ Estrutura do projeto

```text
mla/
├── src/
│   ├── common.c
│   ├── common.h
│   ├── main.c
│   ├── openmp.c
│   ├── openmp.h
│   ├── pthreads1.c
│   ├── pthreads2.c
│   └── serial.c
│
├── Makefile
├── testes.log
└── README.md
```

### Principais arquivos

| Arquivo           | Função                                                                                                             |
| ----------------- | ------------------------------------------------------------------------------------------------------------------ |
| `src/main.c`      | Controla a execução do programa e executa as quatro implementações.                                                |
| `src/common.c`    | Contém funções compartilhadas, como cálculo do Mandelbrot, validação dos argumentos e geração dos dados da imagem. |
| `src/common.h`    | Declara estruturas, constantes e funções compartilhadas.                                                           |
| `src/serial.c`    | Implementação sequencial do cálculo.                                                                               |
| `src/openmp.c`    | Implementação paralela utilizando OpenMP.                                                                          |
| `src/openmp.h`    | Declarações relacionadas à implementação OpenMP.                                                                   |
| `src/pthreads1.c` | Implementação paralela com Pthreads e divisão estática de linhas.                                                  |
| `src/pthreads2.c` | Implementação paralela com Pthreads utilizando distribuição *round-robin*.                                         |
| `Makefile`        | Automatiza a compilação e limpeza do projeto.                                                                      |
| `testes.log`      | Registro dos testes realizados e dos tempos obtidos.                                                               |

## ⚙️ Requisitos

Para compilar o projeto, é necessário um ambiente Linux ou compatível com as ferramentas POSIX utilizadas pelo programa.

### Dependências

* GCC
* Make
* Pthreads
* OpenMP
* Biblioteca matemática (`libm`)

O `Makefile` utiliza:

```text
gcc
-Wall
-O2
-fopenmp
-lpthread
-lm
```

## 🚀 Compilação

Clone o repositório:

```bash
git clone https://github.com/LOLRAPOSO/mla.git
cd mla
```

Compile o projeto utilizando:

```bash
make
```

Após a compilação, será gerado o executável:

```text
mandelbrot
```

Para remover os arquivos gerados pela compilação e pelas execuções:

```bash
make clean
```

## ▶️ Execução

O programa recebe quatro argumentos:

```text
./mandelbrot [largura] [altura] [max_iteracoes] [num_threads]
```

### Exemplo

```bash
./mandelbrot 800 800 1000 4
```

Nesse exemplo:

* `800` → largura da imagem;
* `800` → altura da imagem;
* `1000` → número máximo de iterações;
* `4` → número de threads utilizadas nas implementações paralelas.

O programa valida os argumentos antes de iniciar a execução.

Os limites definidos pelo projeto são:

| Parâmetro |      Limite |
| --------- | ----------: |
| Largura   |   1 – 20000 |
| Altura    |   1 – 20000 |
| Iterações | 1 – 1000000 |
| Threads   |     1 – 256 |

## 🧮 Conjunto de Mandelbrot

Para cada ponto do plano complexo, o programa verifica o comportamento da sequência:

```text
zₙ₊₁ = zₙ² + c
```

onde `c` representa o ponto analisado.

O cálculo é interrompido quando:

* o número máximo de iterações é atingido; ou
* o módulo do número complexo ultrapassa o limite definido.

A quantidade de iterações é posteriormente normalizada para um valor entre `0` e `255`, utilizado para representar a intensidade do pixel.

O programa trabalha sobre a região:

```text
Parte real:       [-2.0, 1.0]
Parte imaginária: [-1.5, 1.5]
```

## ⚡ Estratégias de paralelização

### Serial

A implementação serial percorre todos os pixels utilizando dois loops:

```text
para cada linha
    para cada coluna
        calcular ponto do Mandelbrot
```

Nenhuma thread adicional é criada.

### OpenMP

A implementação OpenMP utiliza:

```c
#pragma omp parallel for
```

para distribuir as linhas da imagem entre as threads.

O projeto utiliza escalonamento:

```text
schedule(dynamic)
```

permitindo distribuir dinamicamente o trabalho entre as threads.

### Pthreads 1 — divisão estática

A primeira implementação com Pthreads divide a imagem em **blocos de linhas**.

Por exemplo, com 4 threads:

```text
Thread 0 → linhas 0–199
Thread 1 → linhas 200–399
Thread 2 → linhas 400–599
Thread 3 → linhas 600–799
```

Quando a quantidade de linhas não é divisível pelo número de threads, as linhas restantes são distribuídas entre as primeiras threads.

### Pthreads 2 — Round-Robin

A segunda implementação distribui as linhas utilizando uma estratégia *round-robin*.

Com 4 threads:

```text
Thread 0 → 0, 4, 8, 12, ...
Thread 1 → 1, 5, 9, 13, ...
Thread 2 → 2, 6, 10, 14, ...
Thread 3 → 3, 7, 11, 15, ...
```

Essa estratégia procura distribuir melhor o trabalho quando determinadas regiões da imagem exigem mais iterações para serem calculadas.

## 📊 Resultados

Em um dos testes registrados no projeto, foi utilizada a seguinte configuração:

```text
Largura:       800
Altura:        800
Iterações:     1000
Threads:       4
```

Os tempos registrados foram:

| Implementação |      Tempo |
| ------------- | ---------: |
| Serial        | 0,535498 s |
| OpenMP        | 0,190536 s |
| Pthreads 1    | 0,201837 s |
| Pthreads 2    | 0,175677 s |

Nesse teste, a implementação **Pthreads 2** apresentou o menor tempo entre as quatro versões.

> Os tempos podem variar de acordo com o processador, sistema operacional, quantidade de threads e carga do sistema durante a execução.

## 🖼️ Arquivos gerados

Após uma execução, o programa gera quatro arquivos correspondentes às implementações:

```text
mandelbrot_mla_serial.pgm
mandelbrot_mla_openmp.pgm
mandelbrot_mla_pthreads1.pgm
mandelbrot_mla_pthreads2.pgm
```

Também é gerado:

```text
times.txt
```

contendo o tempo de execução de cada implementação.

Como as quatro implementações utilizam os mesmos parâmetros matemáticos, os resultados de imagem devem ser equivalentes. No teste registrado no projeto, os quatro arquivos apresentaram o mesmo `MD5`, indicando que produziram exatamente os mesmos dados de saída.

## 🧪 Verificação dos resultados

Depois da execução, os arquivos podem ser comparados utilizando:

```bash
md5sum *.pgm
```

Se as quatro implementações produzirem exatamente o mesmo resultado, os hashes deverão ser iguais.

Os tempos podem ser visualizados com:

```bash
cat times.txt
```

## 🧹 Limpeza

Para remover os arquivos temporários e resultados gerados:

```bash
make clean
```

O comando remove:

```text
*.o
mandelbrot
*.pgm
times.txt
```

## 🎯 Objetivo

O principal objetivo do projeto é analisar, na prática, o impacto da **paralelização de uma tarefa computacionalmente intensiva**.

A implementação permite comparar:

* execução sequencial;
* paralelização automática com OpenMP;
* criação e gerenciamento manual de threads com Pthreads;
* diferentes estratégias de distribuição de trabalho;
* tempo de execução entre as abordagens;
* consistência dos resultados produzidos pelas diferentes implementações.

## 👥 Autoria

Projeto desenvolvido por **LOLRAPOSO**.

## 📄 Licença

Este projeto é disponibilizado para fins acadêmicos e de estudo.
