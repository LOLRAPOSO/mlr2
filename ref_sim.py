#!/usr/bin/env python3
"""Simulador de referencia independente (Python) para validar o ./vm em C.

Uso:
  python3 ref_sim.py comparar    # vm x referencia: varios arquivos e todas as combinacoes
  python3 ref_sim.py dirigidos   # casos feitos a mao (FIFO x LRU, invalidacao da TLB)
  python3 ref_sim.py limites     # enderecos de borda, CRLF, sem newline final, espacos

Cada verificacao imprime OK/FALHOU; o codigo de saida e 1 se houver falha.
"""
import os, random, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
STORE = open(os.path.join(HERE, "BACKING_STORE.bin"), "rb").read()
LINE = re.compile(r"Virtual address: (\d+) Physical address: (\d+) Value: (-?\d+)")
fails = 0
total = 0


def sbyte(i):
    v = STORE[i]
    return v - 256 if v > 127 else v


def sim(addrs, page_alg, tlb_alg, frames=128, use_tlb=True):
    pt, fp, used = {}, [None] * frames, [0] * frames
    nfree = fifo_next = tick = 0
    tlb = [None] * 16
    faults = hits = evic = 0
    out = []
    for a in addrs:
        l = a & 0xFFFF
        page, off = l >> 8, l & 0xFF
        frame = None
        if use_tlb:
            for e in tlb:
                if e and e["page"] == page:
                    frame = e["frame"]; tick += 1; e["last"] = tick; hits += 1
                    break
        if frame is None:
            frame = pt.get(page)
            if frame is None:
                faults += 1
                if nfree < frames:
                    frame = nfree; nfree += 1
                else:
                    if page_alg == "fifo":
                        frame = fifo_next; fifo_next = (fifo_next + 1) % frames
                    else:
                        frame = min(range(frames), key=lambda f: used[f])
                    old = fp[frame]; del pt[old]; evic += 1
                    for i, e in enumerate(tlb):
                        if e and e["page"] == old:
                            tlb[i] = None
                pt[page] = frame; fp[frame] = page
            if use_tlb:
                free = [i for i, e in enumerate(tlb) if e is None]
                if free:
                    v = free[0]
                else:
                    key = "loaded" if tlb_alg == "fifo" else "last"
                    v = min(range(16), key=lambda i: tlb[i][key])
                tick += 1
                tlb[v] = {"page": page, "frame": frame, "loaded": tick, "last": tick}
        tick += 1; used[frame] = tick
        out.append((a, frame * 256 + off, sbyte(l)))
    return out, faults, hits, evic


def run_vm(binary, addr_bytes, page_alg, tlb_alg):
    with tempfile.TemporaryDirectory() as d:
        os.symlink(os.path.join(HERE, "BACKING_STORE.bin"), os.path.join(d, "BACKING_STORE.bin"))
        open(os.path.join(d, "a.txt"), "wb").write(addr_bytes)
        p = subprocess.run([os.path.join(HERE, binary), "a.txt", page_alg, tlb_alg],
                           cwd=d, capture_output=True, text=True)
        lines = open(os.path.join(d, "correct.txt")).read().splitlines() if os.path.exists(os.path.join(d, "correct.txt")) else []
    res = [tuple(int(x) for x in LINE.fullmatch(l).groups()) for l in lines]
    g = lambda k: int(re.search(k + r" = (\d+)", p.stdout).group(1)) if re.search(k + r" = (\d+)", p.stdout) else 0
    return res, g("Page Faults"), g("TLB Hits"), g("Page Replacements"), p.returncode


def check(name, ok, detail=""):
    global fails, total
    total += 1
    if not ok:
        fails += 1
    print(("OK      " if ok else "FALHOU  ") + name + (("  -> " + detail) if detail else ""))


def to_bytes(addrs):
    return ("\n".join(str(a) for a in addrs) + "\n").encode()


def comparar():
    rnd = random.Random(7)
    files = {"address.txt": [int(l) for l in open(os.path.join(HERE, "address.txt")) if l.strip()]}
    loop = []
    for _ in range(6):                      # varredura ciclica de 150 paginas (pior caso FIFO/LRU)
        loop += [p * 256 + rnd.randrange(256) for p in range(150)]
        loop += [rnd.randrange(8) * 256 + rnd.randrange(256) for _ in range(30)]
    files["laco_150_paginas"] = loop
    files["uniforme_2000"] = [rnd.randrange(65536) for _ in range(2000)]
    for fname, addrs in files.items():
        data = to_bytes(addrs)
        for pa in ("fifo", "lru"):
            for ta in ("fifo", "lru"):
                exp = sim(addrs, pa, ta)
                for binary in ("vm", "vm_tlb_nothread"):
                    got = run_vm(binary, data, pa, ta)
                    ok = got[0] == exp[0] and got[1:4] == exp[1:]
                    check(f"{fname:18s} {binary:16s} pagina={pa} tlb={ta}", ok,
                          "" if ok else f"vm faults={got[1]} hits={got[2]} | ref faults={exp[1]} hits={exp[2]}")
            exp = sim(addrs, pa, "fifo", use_tlb=False)
            got = run_vm("vm_128_notlb", data, pa, "fifo")
            check(f"{fname:18s} {'vm_128_notlb':16s} pagina={pa}", got[0] == exp[0] and got[1] == exp[1],
                  f"vm faults={got[1]} ref faults={exp[1]}")
        exp = sim(addrs, "fifo", "fifo", frames=256, use_tlb=False)
        got = run_vm("vm_256_notlb", data, "fifo", "fifo")
        check(f"{fname:18s} {'vm_256_notlb':16s} (256 frames)", got[0] == exp[0] and got[1] == exp[1] and got[3] == 0,
              f"vm faults={got[1]} substituicoes={got[3]}")


def dirigidos():
    # D1: 128 paginas enchem a memoria; pagina 0 e relida; pagina 128 forca substituicao; pagina 0 volta.
    seq = [p * 256 + 5 for p in range(128)] + [5, 128 * 256 + 5, 5]
    data = to_bytes(seq)
    f = run_vm("vm_128_notlb", data, "fifo", "fifo")
    l = run_vm("vm_128_notlb", data, "lru", "fifo")
    print("  (esperado FIFO: pag.128 vai ao frame 0 -> fisico 5; pag.0 volta no frame 1 -> fisico 261; 130 faults)")
    print("  (esperado LRU : pag.128 vai ao frame 1 -> fisico 261; pag.0 ainda residente no frame 0 -> fisico 5; 129 faults)")
    check("D1 FIFO: vitima = frame 0 (pagina mais antiga)", f[0][129][1] == 5, f"fisico obtido={f[0][129][1]}")
    check("D1 FIFO: pagina 0 sofre novo page fault (130 faults)", f[1] == 130 and f[0][130][1] == 261, f"faults={f[1]} fisico={f[0][130][1]}")
    check("D1 LRU : vitima = frame 1 (pagina 0 foi reusada)", l[0][129][1] == 261, f"fisico obtido={l[0][129][1]}")
    check("D1 LRU : pagina 0 continua residente (129 faults)", l[1] == 129 and l[0][130][1] == 5, f"faults={l[1]} fisico={l[0][130][1]}")
    # D2: TLB de 16 entradas; pagina 0 relida; pagina 16 forca troca; pagina 0 de novo.
    seq = [p * 256 + 1 for p in range(16)] + [1, 16 * 256 + 1, 1]
    data = to_bytes(seq)
    tf = run_vm("vm", data, "lru", "fifo")
    tl = run_vm("vm", data, "lru", "lru")
    print("  (esperado TLB FIFO: 1 hit; TLB LRU: 2 hits)")
    check("D2 TLB FIFO: 1 hit (pagina 0 foi a mais antiga e saiu)", tf[2] == 1, f"hits={tf[2]}")
    check("D2 TLB LRU : 2 hits (pagina 1 saiu, pagina 0 ficou)", tl[2] == 2, f"hits={tl[2]}")
    # D3: pagina removida da memoria fisica nao pode continuar valida na TLB.
    seq = [p * 256 + 9 for p in range(128)] + [9, 128 * 256 + 9, 9]
    data = to_bytes(seq)
    r = run_vm("vm", data, "fifo", "lru")
    esperado = sim([int(x) for x in seq], "fifo", "lru")
    ok = r[0] == esperado[0] and r[0][-1][1] == 265 and r[0][-1][2] == sbyte(9)
    check("D3 invalidacao da TLB: pagina 0 recarregada no frame 1 (fisico 256+9=265, valor correto)", ok,
          f"fisico={r[0][-1][1]} valor={r[0][-1][2]} esperado valor={sbyte(9)}")


def limites():
    casos = {
        "0, 255, 256, 65535": [0, 255, 256, 65535],
        "65536 e 4294967295 (truncados para 16 bits)": [65536, 4294967295, 65537],
        "arquivo com 1 unico endereco": [12345],
        "mesmo endereco 3x (hits)": [777, 777, 777],
    }
    for nome, addrs in casos.items():
        exp = sim(addrs, "lru", "fifo")
        got = run_vm("vm", to_bytes(addrs), "lru", "fifo")
        check(nome, got[0] == exp[0] and got[4] == 0, f"saida={got[0]}")
    crlf = "\r\n".join(["100", "20000", "300"]).encode() + b"\r\n"
    got = run_vm("vm", crlf, "lru", "fifo")
    check("arquivo com fim de linha Windows (CRLF)", got[0] == sim([100, 20000, 300], "lru", "fifo")[0] and got[4] == 0)
    got = run_vm("vm", b"100\n200\n300", "lru", "fifo")
    check("arquivo sem quebra de linha no final", len(got[0]) == 3 and got[4] == 0)
    got = run_vm("vm", b"  100  \n200\n\n\n", "lru", "fifo")
    check("espacos ao redor do numero e linhas em branco no final", len(got[0]) == 2 and got[4] == 0)


if __name__ == "__main__":
    modo = sys.argv[1] if len(sys.argv) > 1 else ""
    {"comparar": comparar, "dirigidos": dirigidos, "limites": limites}.get(modo, lambda: sys.exit("uso: ref_sim.py comparar|dirigidos|limites"))()
    print(f"RESUMO ({modo}): {total - fails}/{total} verificacoes OK")
    sys.exit(1 if fails else 0)
