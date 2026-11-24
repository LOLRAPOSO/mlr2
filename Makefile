CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2
LDFLAGS ?=
LIBS    = -pthread

# "make" gera o executavel vm (128 frames, TLB com threads)
all: vm

vm: vm.c
	$(CC) $(CFLAGS) -pthread -o vm vm.c $(LDFLAGS) $(LIBS)

# Variantes usadas apenas para testar os fluxos da rubrica
variants: vm_256_notlb vm_128_notlb vm_tlb_nothread

vm_256_notlb: vm.c
	$(CC) $(CFLAGS) -pthread -DFRAMES=256 -DUSE_TLB=0 -o $@ vm.c $(LDFLAGS) $(LIBS)

vm_128_notlb: vm.c
	$(CC) $(CFLAGS) -pthread -DUSE_TLB=0 -o $@ vm.c $(LDFLAGS) $(LIBS)

vm_tlb_nothread: vm.c
	$(CC) $(CFLAGS) -pthread -DTLB_THREADS=0 -o $@ vm.c $(LDFLAGS) $(LIBS)

# Dados de teste (apenas se voce nao tiver os arquivos do professor)
testdata:
	python3 gen_data.py

# Roda a sessao de testes (testes.sh) e acrescenta a saida em evidencias.log
evidencias: vm variants
	sh testes.sh 2>&1 | tee -a evidencias.log

clean:
	rm -f vm vm_256_notlb vm_128_notlb vm_tlb_nothread correct.txt out_*.txt

.PHONY: all variants testdata evidencias clean
