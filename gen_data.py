#!/usr/bin/env python3
"""Gera dados de teste: BACKING_STORE.bin (65536 bytes) e address.txt (1000 enderecos).

Use SOMENTE se voce nao tiver os arquivos originais do professor
(address.txt e BACKING_STORE.bin). Os do professor tem prioridade.
Os enderecos tem localidade temporal (conjunto de trabalho que muda ao longo
do tempo) para que FIFO e LRU produzam resultados diferentes.
"""
import random

random.seed(2025)
store = bytes(random.randrange(256) for _ in range(65536))
with open("BACKING_STORE.bin", "wb") as f:
    f.write(store)

addrs = []
hot = random.sample(range(256), 90)           # conjunto de trabalho
for i in range(1000):
    if i % 100 == 0:                          # o conjunto de trabalho muda
        hot = random.sample(range(256), 90)
    if random.random() < 0.75:
        page = random.choice(hot)
    else:
        page = random.randrange(256)
    addrs.append(page * 256 + random.randrange(256))

with open("address.txt", "w") as f:
    f.write("\n".join(str(a) for a in addrs) + "\n")
print("Gerados BACKING_STORE.bin e address.txt")
