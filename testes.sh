#!/bin/sh
# Sessao de testes da Implementacao 6. Tudo vai para stdout, com horario em cada passo.
# Uso:  make evidencias   (equivale a: sh testes.sh 2>&1 | tee -a evidencias.log)
FALHAS=0
T() { date '+[%H:%M:%S]'; }
passo() { echo; echo "$(T) ##### $*"; }
run() { echo "$(T) \$ $*"; "$@"; rc=$?; echo "$(T) (saida=$rc)"; return $rc; }
sh_run() { echo "$(T) \$ $*"; sh -c "$*"; rc=$?; echo "$(T) (saida=$rc)"; }
falha() { FALHAS=$((FALHAS + 1)); echo "$(T) *** FALHA: $*"; }
W=$(mktemp -d)

echo "================================================================"
echo "SESSAO DE TESTES - Implementacao 6 (Memoria Virtual)"
run date
run whoami
run pwd
run uname -srm
sh_run "gcc --version | head -1"

passo "1. [S1] Compilar do zero (comandos do relatorio, secao 3)"
run make clean
run make
run ls -l vm

passo "2. Dados de teste (sinteticos, gen_data.py; os do professor nao estavam disponiveis)"
[ -f address.txt ] && [ -f BACKING_STORE.bin ] || run make testdata
sh_run "wc -l address.txt; ls -l BACKING_STORE.bin"

passo "3. [S1] Exemplo do enunciado: ./vm address.txt lru fifo"
run ./vm address.txt lru fifo
sh_run "head -3 correct.txt; wc -l correct.txt"

passo "4. [S1] Variantes da rubrica (256 frames, sem TLB, TLB sem threads)"
sh_run "make variants 2>&1 | grep -c -i warning; true"
run ls -l vm_256_notlb vm_128_notlb vm_tlb_nothread

passo "5. Reproducao do aviso de compilacao encontrado nas variantes sem TLB (corrigido no vm.c)"
echo "     (copia do vm.c SEM o atributo 'unused' compilada com -DUSE_TLB=0 -Wall -Wextra)"
sed 's/__attribute__((unused)) //' vm.c > "$W/vm_antigo.c"
sh_run "gcc -Wall -Wextra -O2 -pthread -DUSE_TLB=0 -o $W/vm_antigo $W/vm_antigo.c 2>&1 | grep warning"
echo "     (vm.c atual, mesmas flags:)"
sh_run "gcc -Wall -Wextra -O2 -pthread -DUSE_TLB=0 -o $W/vm_novo vm.c 2>&1 | grep -c warning; true"

passo "6. [S1] Os 9 fluxos da rubrica (estatisticas)"
fluxo() { # fluxo <n> <binario> <pagina> <tlb>
    echo "$(T) \$ ./$2 address.txt $3 $4   (fluxo $1)"
    ./"$2" address.txt "$3" "$4" | grep -E "Configuracao|Page Faults =|Page Replacements|TLB Hits ="
    cp correct.txt "$W/fluxo$1.txt"
}
fluxo 1 vm_256_notlb fifo fifo
fluxo 2 vm_128_notlb fifo fifo
fluxo 3 vm_128_notlb lru fifo
fluxo 4 vm_tlb_nothread lru fifo
fluxo 5 vm_tlb_nothread lru lru
fluxo 6 vm lru fifo
fluxo 7 vm lru lru
fluxo 8 vm fifo fifo
fluxo 9 vm fifo lru

passo "7. Conferencia: valor lido x BACKING_STORE.bin e saidas com TLB x sem TLB"
for n in 1 2 3 4 5 6 7 8 9; do
python3 - "$W/fluxo$n.txt" <<'EOF' || falha "fluxo $n difere do BACKING_STORE"
import re, sys
store = open("BACKING_STORE.bin", "rb").read()
addrs = [int(l) for l in open("address.txt") if l.strip()]
lines = open(sys.argv[1]).read().splitlines()
bad = len(lines) != len(addrs)
for a, l in zip(addrs, lines):
    m = re.fullmatch(r"Virtual address: (\d+) Physical address: (\d+) Value: (-?\d+)", l)
    v = store[a & 0xFFFF]; v = v - 256 if v > 127 else v
    bad |= not m or int(m[1]) != a or int(m[3]) != v or (int(m[2]) & 0xFF) != (a & 0xFF)
print(f"fluxo {sys.argv[1][-5]}: {len(lines)} linhas, ", "FALHOU" if bad else "valores e deslocamentos OK")
sys.exit(1 if bad else 0)
EOF
done
cmp -s "$W/fluxo3.txt" "$W/fluxo6.txt" && echo "fluxo 3 (sem TLB) == fluxo 6 (com TLB e threads): saidas IGUAIS" || falha "fluxo 3 != fluxo 6"
cmp -s "$W/fluxo4.txt" "$W/fluxo6.txt" && echo "fluxo 4 (TLB sem thread) == fluxo 6 (TLB com threads): saidas IGUAIS" || falha "fluxo 4 != fluxo 6"
cmp -s "$W/fluxo5.txt" "$W/fluxo7.txt" && echo "fluxo 5 (TLB sem thread) == fluxo 7 (TLB com threads): saidas IGUAIS" || falha "fluxo 5 != fluxo 7"

passo "8. Simulador de referencia independente (Python) x vm: 3 arquivos, todas as combinacoes"
run python3 ref_sim.py comparar || falha "ref_sim comparar"

passo "9. Testes dirigidos (resultado calculado a mao): FIFO x LRU em paginas e TLB, invalidacao da TLB"
python3 ref_sim.py dirigidos || falha "ref_sim dirigidos"

passo "10. Casos limite"
python3 ref_sim.py limites || falha "ref_sim limites"

passo "11. Casos invalidos (mensagem coerente + codigo de saida 1)"
printf '1\n2\nabc\n' > "$W/texto.txt"; printf '100\n\n200\n' > "$W/branco.txt"
printf '100\n-5\n' > "$W/neg.txt"; printf '99999999999\n' > "$W/grande.txt"; : > "$W/vazio.txt"
erro() { # erro <titulo> <args...>  (espera saida 1 e nenhum correct.txt novo)
    t=$1; shift; rm -f correct.txt
    echo "$(T) --- $t"; echo "$(T) \$ ./vm $*"
    ./vm "$@" 2>&1 | head -2; rc=$(./vm "$@" >/dev/null 2>&1; echo $?)
    [ "$rc" = 1 ] && [ ! -f correct.txt ] && echo "     -> saida=1 e correct.txt nao criado: OK" || falha "$t (saida=$rc)"
}
erro "E1 sem argumentos"
erro "E2 argumentos a menos" address.txt lru
erro "E3 argumentos a mais" address.txt lru fifo extra
erro "E4 algoritmo de pagina invalido" address.txt xyz fifo
erro "E5 algoritmo de TLB invalido" address.txt lru 123
erro "E6 arquivo inexistente" naoexiste.txt lru fifo
erro "E7 arquivo e um diretorio" "$W" lru fifo
erro "E8 linha nao numerica" "$W/texto.txt" lru fifo
erro "E9 linha em branco no meio" "$W/branco.txt" lru fifo
erro "E10 numero negativo" "$W/neg.txt" lru fifo
erro "E11 numero acima de 32 bits" "$W/grande.txt" lru fifo
erro "E12 arquivo vazio" "$W/vazio.txt" lru fifo
mv BACKING_STORE.bin BACKING_STORE.bak
erro "E13 BACKING_STORE.bin ausente" address.txt lru fifo
mv BACKING_STORE.bak BACKING_STORE.bin

passo "12. Sanitizers: ThreadSanitizer (corrida de dados) e Valgrind (memoria)"
sh_run "gcc -g -O1 -fsanitize=thread -pthread -o $W/vm_tsan vm.c && $W/vm_tsan address.txt lru fifo 2>&1 | grep -E 'WARNING|ThreadSanitizer|Page Faults|TLB Hits ='"
echo "     (esperado: nenhum WARNING do ThreadSanitizer; mesmos Page Faults/TLB Hits dos fluxos 6)"
head -40 address.txt > "$W/pequeno.txt"
echo "$(T) \$ valgrind -q --error-exitcode=9 --leak-check=full ./vm pequeno.txt lru fifo   (40 enderecos)"
valgrind -q --error-exitcode=9 --leak-check=full ./vm "$W/pequeno.txt" lru fifo > "$W/vg_out.txt" 2> "$W/vg_err.txt"; rc=$?
echo "     codigo de saida do valgrind = $rc (0 = nenhum erro de memoria); linhas em stderr: $(wc -l < "$W/vg_err.txt")"
[ "$rc" = 0 ] || falha "valgrind encontrou erro"
tsan_out=$("$W/vm_tsan" address.txt lru fifo 2>&1 | grep -c "WARNING: ThreadSanitizer")
[ "$tsan_out" = 0 ] || falha "ThreadSanitizer encontrou corrida"

passo "13. Desempenho: TLB com threads x TLB sem threads (5000 enderecos aleatorios)"
python3 -c "
import random; r=random.Random(3)
open('$W/grande_5000.txt','w').write('\n'.join(str(r.randrange(65536)) for _ in range(5000))+'\n')"
bench() { s=$(date +%s%N); ./"$1" "$W/grande_5000.txt" lru fifo >/dev/null; e=$(date +%s%N); echo "$1: $(( (e - s) / 1000000 )) ms"; }
bench vm_128_notlb
bench vm_tlb_nothread
bench vm
bench vm_tlb_nothread
bench vm

passo "RESULTADO FINAL DA SESSAO"
rm -f correct.txt
rm -rf "$W"
if [ "$FALHAS" = 0 ]; then echo "$(T) Todas as verificacoes passaram (0 falhas)."; else echo "$(T) FALHAS: $FALHAS"; fi
exit "$FALHAS"
