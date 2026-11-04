# Generated: MIPS variant comparison

Produced by `tools/target-build/compare-variants.py` from the binaries in `build-target/`. **Describes our compiler's output only; says nothing about the ZR36473.**

## hello_s650

| endian | -march | ELF arch flag | text bytes | instructions | distinct mnemonics | beyond MIPS-I (lower bound) | lwl/lwr/swl/swr | calls (jal/jalr) |
|---|---|---|---|---|---|---|---|---|
| little | mips1 | mips1 | 96 | 22 | 6 | mips1  | 0 | 0 |
| little | mips32 | mips32 | 96 | 22 | 6 | mips1  | 0 | 0 |
| little | mips32r2 | mips32r2 | 96 | 22 | 6 | mips1  | 0 | 0 |
| big | mips1 | mips1 | 96 | 22 | 6 | mips1  | 0 | 0 |
| big | mips32 | mips32 | 96 | 22 | 6 | mips1  | 0 | 0 |
| big | mips32r2 | mips32r2 | 96 | 22 | 6 | mips1  | 0 | 0 |

**Byte order relation** (same ISA): is the big-endian `.text` the word-swapped little-endian `.text`?

- mips1: identical after swapping each 32-bit word; same instruction sequence: True
- mips32: identical after swapping each 32-bit word; same instruction sequence: True
- mips32r2: identical after swapping each 32-bit word; same instruction sequence: True

**ISA relation** (same byte order): how does the code change between `-march` levels?

- little: mips1 -> mips32: text 96 -> 96 bytes; instructions 22 -> 22; mnemonics only in mips32: none; only in mips1: none
- little: mips1 -> mips32r2: text 96 -> 96 bytes; instructions 22 -> 22; mnemonics only in mips32r2: none; only in mips1: none
- big: mips1 -> mips32: text 96 -> 96 bytes; instructions 22 -> 22; mnemonics only in mips32: none; only in mips1: none
- big: mips1 -> mips32r2: text 96 -> 96 bytes; instructions 22 -> 22; mnemonics only in mips32r2: none; only in mips1: none

## mips_sample

| endian | -march | ELF arch flag | text bytes | instructions | distinct mnemonics | beyond MIPS-I (lower bound) | lwl/lwr/swl/swr | calls (jal/jalr) |
|---|---|---|---|---|---|---|---|---|
| little | mips1 | mips1 | 776 | 192 | 28 | mips1  | 0 | 3 |
| little | mips32 | mips32 | 720 | 178 | 27 | mips32 ['mul'] | 0 | 3 |
| little | mips32r2 | mips32r2 | 720 | 178 | 27 | mips32 ['mul'] | 0 | 3 |
| big | mips1 | mips1 | 776 | 192 | 28 | mips1  | 0 | 3 |
| big | mips32 | mips32 | 720 | 178 | 27 | mips32 ['mul'] | 0 | 3 |
| big | mips32r2 | mips32r2 | 720 | 178 | 27 | mips32 ['mul'] | 0 | 3 |

**Byte order relation** (same ISA): is the big-endian `.text` the word-swapped little-endian `.text`?

- mips1: 2 of 194 words differ after swapping (data/immediates or different codegen); same instruction sequence: True
- mips32: 2 of 180 words differ after swapping (data/immediates or different codegen); same instruction sequence: True
- mips32r2: 2 of 180 words differ after swapping (data/immediates or different codegen); same instruction sequence: True

**ISA relation** (same byte order): how does the code change between `-march` levels?

- little: mips1 -> mips32: text 776 -> 720 bytes; instructions 192 -> 178; mnemonics only in mips32: ['mul']; only in mips1: ['mflo', 'mult']
- little: mips1 -> mips32r2: text 776 -> 720 bytes; instructions 192 -> 178; mnemonics only in mips32r2: ['mul']; only in mips1: ['mflo', 'mult']
- big: mips1 -> mips32: text 776 -> 720 bytes; instructions 192 -> 178; mnemonics only in mips32: ['mul']; only in mips1: ['mflo', 'mult']
- big: mips1 -> mips32r2: text 776 -> 720 bytes; instructions 192 -> 178; mnemonics only in mips32r2: ['mul']; only in mips1: ['mflo', 'mult']

## isa_probe

| endian | -march | ELF arch flag | text bytes | instructions | distinct mnemonics | beyond MIPS-I (lower bound) | lwl/lwr/swl/swr | calls (jal/jalr) |
|---|---|---|---|---|---|---|---|---|
| little | mips1 | mips1 | 240 | 54 | 14 | mips32 ['clz', 'movn', 'mul'] | 2 | 0 |
| little | mips32 | mips32 | 240 | 54 | 14 | mips32 ['clz', 'movn', 'mul'] | 2 | 0 |
| little | mips32r2 | mips32r2 | 240 | 59 | 19 | mips32r2 ['clz', 'ext', 'ins', 'movn', 'mul', 'rotr', 'seb', 'wsbh'] | 2 | 0 |
| big | mips1 | mips1 | 240 | 54 | 14 | mips32 ['clz', 'movn', 'mul'] | 2 | 0 |
| big | mips32 | mips32 | 240 | 54 | 14 | mips32 ['clz', 'movn', 'mul'] | 2 | 0 |
| big | mips32r2 | mips32r2 | 240 | 59 | 19 | mips32r2 ['clz', 'ext', 'ins', 'movn', 'mul', 'rotr', 'seb', 'wsbh'] | 2 | 0 |

**Byte order relation** (same ISA): is the big-endian `.text` the word-swapped little-endian `.text`?

- mips1: 2 of 60 words differ after swapping (data/immediates or different codegen); same instruction sequence: False
- mips32: 2 of 60 words differ after swapping (data/immediates or different codegen); same instruction sequence: False
- mips32r2: 2 of 60 words differ after swapping (data/immediates or different codegen); same instruction sequence: False

**ISA relation** (same byte order): how does the code change between `-march` levels?

- little: mips1 -> mips32: text 240 -> 240 bytes; instructions 54 -> 54; mnemonics only in mips32: none; only in mips1: none
- little: mips1 -> mips32r2: text 240 -> 240 bytes; instructions 54 -> 59; mnemonics only in mips32r2: ['ext', 'ins', 'rotr', 'seb', 'wsbh']; only in mips1: none
- big: mips1 -> mips32: text 240 -> 240 bytes; instructions 54 -> 54; mnemonics only in mips32: none; only in mips1: none
- big: mips1 -> mips32r2: text 240 -> 240 bytes; instructions 54 -> 59; mnemonics only in mips32r2: ['ext', 'ins', 'rotr', 'seb', 'wsbh']; only in mips1: none

## Link address effect

`placeholder` (0x80000000) and `qemu` (0x80100000) builds differ only in absolute address immediates (`lui`/`ori`/`j`/`jal`). Words differing between them (mips_sample, little, mips32):

18 of 180 words (all other instructions identical).
