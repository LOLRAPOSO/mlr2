# Implementação 6 — Projetando uma Memória Virtual

Infraestrutura de Software — CESAR School

Simulador de gerenciador de memória virtual em C, baseado no exercício *Designing a Virtual Memory Manager*
(Silberschatz et al., *Operating System Concepts*, 10ª ed., p. P-51).

## Características

- Espaço de endereçamento lógico de 16 bits: 256 páginas de 256 bytes (8 bits de página + 8 bits de deslocamento).
- Memória física com **128 frames**, preenchidos de 0 a 127; depois de cheia, aplica-se o algoritmo de substituição.
- Substituição de **página**: `fifo` ou `lru`.
- Substituição da **TLB** (16 entradas): `fifo` ou `lru`.
- Busca na TLB com **uma thread (pthread) por entrada**.
- Tratamento de erros: argumentos e arquivo de endereços são validados antes de qualquer processamento.

## Compilação

```sh
make          # gera o executável ./vm
make clean    # remove executáveis e correct.txt
```

Requer `gcc` (ou outro compilador C com suporte a POSIX threads) em Linux, Unix ou macOS.

## Execução

```sh
./vm <arquivo_de_enderecos> <fifo|lru> <fifo|lru>
#       1º arg: endereços lógicos   2º: algoritmo de página   3º: algoritmo da TLB
```

Exemplo: `./vm address.txt lru fifo` (página com LRU, TLB com FIFO).

Arquivos necessários no diretório de execução:

| Arquivo | Descrição |
|---|---|
| arquivo de endereços (ex.: `address.txt`) | um endereço lógico (inteiro não negativo) por linha |
| `BACKING_STORE.bin` | armazenamento secundário, exatamente 65536 bytes (fornecido pelo professor) |

Saída: `correct.txt`, uma linha por endereço, no formato

```
Virtual address: 16916 Physical address: 20 Value: 0
```

Ao final, as estatísticas (page faults, substituições, TLB hits) são impressas no terminal.

## Estrutura do código (`vm.c`)

- `page_table[256]`: página → frame (−1 se não residente); `frame_page[128]`: frame → página.
- **FIFO de páginas**: ponteiro circular `fifo_next`; como os frames são ocupados em ordem 0..127, o frame mais antigo é sempre o apontado.
- **LRU de páginas**: carimbo lógico `frame_used[]` atualizado a cada acesso (inclusive hits na TLB); a vítima é o menor carimbo.
- **TLB**: 16 entradas com `loaded_at` (FIFO) e `last_used` (LRU). Quando uma página é retirada da memória física, sua entrada na TLB é invalidada.
- **Busca com threads**: para cada endereço, 16 threads são criadas (uma por entrada); cada uma examina só a sua entrada e grava o resultado em uma estrutura própria (sem condição de corrida); após o `pthread_join`, o resultado é consolidado.

## Tratamento de erros

O programa encerra com mensagem clara e código de saída 1 quando:

1. o número de argumentos for diferente de 3;
2. o 2º ou o 3º argumento não for `fifo` ou `lru`;
3. o arquivo de endereços não existir, não puder ser aberto/lido, estiver vazio ou tiver formato inválido
   (linha não numérica, número negativo, valor acima de 32 bits ou linha em branco antes de outra linha);
4. `BACKING_STORE.bin` não existir ou não tiver 65536 bytes.

Todo o arquivo de endereços é validado **antes** de processar o primeiro endereço (nenhum `correct.txt` parcial é gerado).

## Variantes de teste (rubrica)

Para exercitar cada fluxo da rubrica separadamente, o Makefile gera binários extras (o `./vm` entregue não muda):

```sh
make variants    # vm_256_notlb, vm_128_notlb, vm_tlb_nothread
make evidencias  # roda testes.sh (usa ref_sim.py) e acrescenta a saída ao evidencias.log
```

| Binário | Fluxo |
|---|---|
| `vm_256_notlb` | sem TLB, 256 frames (sem substituição) |
| `vm_128_notlb` | sem TLB, 128 frames, FIFO ou LRU |
| `vm_tlb_nothread` | com TLB, busca sequencial (sem threads), FIFO ou LRU |
| `vm` | com TLB e threads (versão final) |

## Dados de teste

Se não tiver os arquivos do professor, `make testdata` gera um `BACKING_STORE.bin` e um `address.txt` sintéticos
(`gen_data.py`). Os arquivos do professor devem ser usados sempre que disponíveis.

## Evidências

`evidencias.log` (gerado por `make evidencias`, com horário em cada passo) contém as execuções de todos os fluxos, a conferência automática dos valores lidos contra o
`BACKING_STORE.bin` e os testes de erro.
