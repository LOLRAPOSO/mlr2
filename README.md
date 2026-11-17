Algoritmo de Banker — Gerenciamento de Deadlocks

## Sobre o projeto

Este projeto implementa, em linguagem C, o **Algoritmo de Banker (Banqueiro)**, utilizado para analisar solicitações de recursos e evitar que o sistema entre em um estado inseguro.

A implementação faz parte da atividade de Infraestrutura de Software — Implementação 5. O programa recebe a quantidade de instâncias disponíveis de cada tipo de recurso pela linha de comando, lê os dados dos clientes e executa uma sequência de solicitações e liberações descritas em arquivo.

## Objetivo

Simular o gerenciamento de recursos de um sistema com múltiplos clientes, verificando se uma solicitação pode ser atendida sem comprometer a segurança do estado do sistema.

Durante a execução, o programa mantém as estruturas:

- **Available:** quantidade de instâncias disponíveis de cada recurso.
- **Maximum:** quantidade máxima de cada recurso que cada cliente pode solicitar.
- **Allocation:** quantidade de recursos atualmente alocada para cada cliente.
- **Need:** quantidade de recursos que ainda pode ser necessária por cada cliente (`Maximum - Allocation`).

Antes de confirmar uma solicitação, o programa simula a alocação e executa a verificação de estado seguro. Se o estado resultante for inseguro, a solicitação é desfeita e negada.

## Tecnologias

- Linguagem C
- GCC
- Makefile
- Terminal Linux, Unix ou macOS

O Makefile utiliza as opções `-Wall -Wextra -std=c99 -O2` na compilação.

## Estrutura do repositório

| Arquivo | Descrição |
|---|---|
| `banker.c` | Código-fonte da implementação do Algoritmo de Banker. |
| `Makefile` | Automatiza a compilação e a limpeza dos arquivos gerados. |
| `customer.txt` | Define a quantidade máxima de recursos que cada cliente pode solicitar. |
| `commands.txt` | Contém a sequência de solicitações, liberações e comandos de exibição do estado. |
| `evidencias.log` | Arquivo de evidências da atividade. |
| `banker` | Executável gerado pelo processo de compilação. |
| `result.txt` | Arquivo de saída gerado durante a execução. |

Os arquivos `banker`, `result.txt` e os arquivos de entrada devem estar no diretório de execução do programa.

## Como compilar

É necessário ter o GCC e o Make instalados.

No terminal, entre no diretório do projeto e execute:

```bash
make
```

O comando compila `banker.c` e gera o executável `banker`.

Para remover o executável e o arquivo de saída gerado, execute:

```bash
make clean
```

## Como executar

A quantidade de argumentos informados na execução determina a quantidade de tipos de recursos. Cada argumento representa o número inicial de instâncias disponíveis de um tipo de recurso.

Formato:

```bash
./banker <recurso_0> <recurso_1> ... <recurso_n>
```

Exemplo com três tipos de recursos:

```bash
./banker 10 5 7
```

Nesse exemplo, o sistema começa com 10 instâncias do primeiro recurso, 5 do segundo e 7 do terceiro.

Antes de executar, mantenha os arquivos `customer.txt` e `commands.txt` no mesmo diretório do executável.

## Arquivo `customer.txt`

O arquivo `customer.txt` informa a quantidade máxima de cada tipo de recurso que cada cliente poderá solicitar.

Cada linha representa um cliente, e cada valor separado por vírgula representa o máximo de um tipo de recurso. Portanto:

- O número de linhas define a quantidade de clientes.
- A quantidade de valores em cada linha deve corresponder à quantidade de recursos informada na execução.
- Os valores representam as demandas máximas dos clientes.

Exemplo para três tipos de recursos:

```text
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

Nesse exemplo, existem cinco clientes e três tipos de recursos.

## Arquivo `commands.txt`

O arquivo `commands.txt` contém os comandos que serão executados pelo programa, na ordem em que aparecem.

São utilizados três comandos:

| Comando | Funcionamento |
|---|---|
| `RQ` | Solicita recursos para um cliente. |
| `RL` | Libera recursos previamente alocados a um cliente. |
| `*` | Exibe o estado atual das estruturas Maximum, Allocation, Need e Available. |

### Solicitação de recursos — `RQ`

Formato:

```text
RQ <cliente> <recurso_0> <recurso_1> ... <recurso_n>
```

Exemplo:

```text
RQ 0 0 2 1
```

O cliente `0` solicita 0 instâncias do primeiro recurso, 2 do segundo e 1 do terceiro.

A solicitação é negada quando ultrapassa a necessidade máxima restante do cliente, quando não há recursos disponíveis suficientes ou quando a alocação resultaria em um estado inseguro.

### Liberação de recursos — `RL`

Formato:

```text
RL <cliente> <recurso_0> <recurso_1> ... <recurso_n>
```

Exemplo:

```text
RL 0 0 1 1
```

O cliente `0` libera 0 instâncias do primeiro recurso, 1 do segundo e 1 do terceiro. Os recursos liberados retornam ao conjunto de recursos disponíveis.

### Exibição do estado — `*`

Uma linha contendo somente `*` solicita a impressão do estado atual do sistema no arquivo de saída.

Exemplo de sequência de comandos:

```text
RQ 0 0 2 1
RQ 2 3 0 2
RQ 1 2 0 0
RL 0 0 1 1
*
```

## Arquivo de saída `result.txt`

Ao executar o programa, é criado o arquivo `result.txt`. Nele são registradas as operações realizadas e as solicitações negadas, além das tabelas de estado solicitadas pelo comando `*`.

A exibição do estado apresenta:

- **MAXIMUM:** demanda máxima de cada cliente.
- **ALLOCATION:** recursos atualmente alocados.
- **NEED:** recursos que ainda podem ser solicitados.
- **AVAILABLE:** recursos disponíveis no sistema.

As mensagens de saída informam se os recursos foram alocados ou liberados e, quando uma solicitação é negada, apresentam o motivo identificado pelo programa.

## Tratamento de erros

O programa verifica condições de entrada antes de prosseguir com a execução. Entre as situações tratadas estão:

- Ausência de argumentos para informar os recursos.
- Argumentos de recursos inválidos ou negativos.
- Impossibilidade de abrir `customer.txt` ou `commands.txt`.
- Arquivo `customer.txt` vazio.
- Quantidade de valores por cliente diferente da quantidade de recursos informada.
- Valores inválidos no arquivo `customer.txt`.
- Comandos desconhecidos ou incompletos em `commands.txt`.
- Identificador de cliente inexistente.
- Quantidade de valores de recursos diferente da esperada em um comando.
- Impossibilidade de criar `result.txt`.

Quando um erro fatal de entrada é identificado, o programa apresenta uma mensagem no terminal e encerra a execução.

## Limpeza dos arquivos gerados

Para remover o executável e o arquivo `result.txt`, utilize:

```bash
make clean
```

## Atividade

**Disciplina:** Infraestrutura de Software  
**Implementação:** 5 — Algoritmo de Banker  
**Repositório:** [LOLRAPOSO/mlr2](https://github.com/LOLRAPOSO/mlr2)
