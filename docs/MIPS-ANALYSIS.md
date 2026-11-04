# MIPS target analysis (round 9)

Scope: narrow the *hypotheses* for the ZR36473's CPU configuration using (a) available documents and (b) measurements of our own toolchain output and of generic emulated cores. **Nothing here identifies the S650's CPU, and no binary is "the S650 binary".** Generic MIPS is not treated as interchangeable with the ZR36473.

Reproduce: `tools/target-build/build.sh`, `compare-variants.py`, `qemu-isa-matrix.py`, `tools/firmware-analysis/tests/test_toolkit.py`. Full generated tables: `docs/generated/variant-comparison.md`; raw emulator output: `build-target/qemu-isa-matrix.txt`.

## 1. Which CPU variants are consistent with the evidence?
| Hypothesis | Evidence for | Evidence against | Status |
|---|---|---|---|
| 32-bit MIPS CPU | COACH 8/9/10 briefs: "32-bit MIPS CPU" | none | **F** (family) |
| MIPS32 4KE-family core | Zoran licensed the 4KE family in 2002 for COACH/DTV and said it would "transition away from Lexra" cores | The licence precedes COACH 9 (2006-07) by 4+ years and names no product; Zoran may have licensed other cores since | **O/H**, plausible, unproven |
| Release 1 (4KE as licensed in 2002) | Vendor notes (search results, not read in full) say Release 2 features arrived in later 4KE core releases (e.g. "4KEcR2"); QEMU has both `4KEcR1` and `4KEc` (R2) models | A core licensed later could be R2 | **H**: R1 is the *safer* assumption |
| Release 2 | A newer core would have it | no direct evidence | H |
| MIPS-I-only / Lexra-style core (no `lwl/lwr`, possibly no `mul`) | Pre-2002 COACH generations used other cores | Zoran moved away from Lexra in 2002; the brief says "MIPS" | **H, least likely** but the only reason to keep `-march=mips1` as a fallback |
| Little endian / big endian | none | none | **U**, both built |
| FPU present | 4KE-class cores have none | unverified for the 9S | H: soft-float |
| MMU (TLB) vs fixed mapping | 4KEc has a TLB; 4KEm/4KEp do not | unverified | U |

## 2. What the compiler flags can change (measured)
Source: `docs/generated/variant-comparison.md`, 3 programs x 12 variants.

| Flag | Effect on emitted code (measured) | Incompatibility risk |
|---|---|---|
| `-march=mips1` | 3-operand `mul` replaced by `mult` + `mflo` (mips_sample: 776 vs 720 text bytes); no `movn/movz/clz`; ELF flag `mips1` | None on any MIPS core |
| `-march=mips32` | Uses `mul` (MIPS32 R1). No Release 2 instruction appears in any compiled sample | Illegal on a MIPS-I-only core |
| `-march=mips32r2` | **No difference in generated instructions for our samples** (only the ELF flag changes); R2 instructions (`ext ins seb wsbh rotr`) appear only where code uses those idioms (the probe forces them with `.set`) | Illegal on a Release 1 core when such idioms are used - a risk that grows with larger programs |
| `-mabi=32 -msoft-float` | No FP instructions at all; 64-bit multiply and 32-bit divide use `multu/divu`; no libgcc calls were needed for the samples | Floating-point code would call a soft-float library (not present) |
| `-mno-abicalls -fno-pic -G0` | Absolute addressing (`lui/ori`), no `$gp` use, no `.got` use in our code (the linker still emits an empty 8-byte `.got` for o32) | Needs no loader/GOT |
| Endianness (`mipsel` vs `mips`) | Identical instruction sequences; `.text` of big-endian equals the word-swapped little-endian `.text` except 2 words per sample (data/immediate layout); in `isa_probe` the `lwl/lwr` pairing differs by design | An image built for the wrong byte order is garbage to the other core |
| Link address | `0x80000000` vs `0x80100000` change only absolute-address words (`lui`/`ori`/`j`/`jal`): 18 of 180 words for mips_sample | none (placeholder anyway) |
| Unaligned access | Clang emitted **no** `lwl/lwr/swl/swr` for the packed unaligned read (byte-wise code instead) at any `-march` | Good: our code does not depend on those instructions, which some cores lack |
| Flags **not** used | `-mips16`, `-mmicromips`, `-mdsp`, `-mfp64`, `-mcpu`/`-mtune` naming a 4KE | MIPS16e/microMIPS would be a different ISA encoding; naming a specific core would be an unsupported claim |

**hello_s650 is identical across all 12 variants** (22 instructions, same mnemonics), so it cannot discriminate any hypothesis. That is why `mips_sample` and `isa_probe` were added.

## 3. Emulation evidence (QEMU 11.0.2, `malta` board; NOT the ZR36473)
`qemu-isa-matrix.py` results (raw: `build-target/qemu-isa-matrix.txt`):
- **mips_sample, all 6 `*-qemu` variants x 4 CPU models (4KEc R2, 4KEcR1, 4KEmR1, 4Kc): 24/24 pass**, results equal independent host computation (insertion sort, bitwise CRC-32 = zlib, recursion, jump table, function pointers, unaligned read, 64-bit multiply, 32-bit divide). So the compiler output for all hypotheses runs on both R1 and R2 generic cores.
- **isa_probe** (10 instruction-class probes, markers show how far a core gets), both byte orders, every MIPS32-class model:

| QEMU CPU model | ISA level | Probes executed (of 10) |
|---|---|---|
| 4Kc, 4Km, 4KEcR1, 4KEmR1, XBurstR1 | MIPS32 Release 1 | **4** (stop at the first Release 2 instruction `ext`) |
| 4KEc, 4KEm, 24Kc/24KEc/24Kf, 34Kf, 74Kf, M14K/M14Kc, P5600, XBurstR2 | MIPS32 Release 2 (or newer) | 10 |
| mips32r6-generic | Release 6 | 1 (R6 removed the old `mul`/`lwl` encodings) |
| I7200 | MIPS64 R6 | 0 (not a MIPS32 boot target here) |

**What this means:** an R2-using binary distinguishes R1 from R2 cores in a way our hello does not. The probe is a prepared *discriminating experiment* for the day any legitimate execution path exists; it was never run on the camera.
**QEMU model choice:** for a 2002-era 4KE-class guess the closest generic models are `4KEcR1` (R1, TLB) and `4KEmR1` (R1, fixed mapping); for a newer core `4KEc` (R2). None models the ZR36473's peripherals, boot ROM, memory map or endianness setting.

## 4. Toolkit-assisted check of our own output
The firmware-analysis toolkit (`docs/FIRMWARE-ANALYSIS-TOOLKIT.md`) classifies our binaries without being told the answer: on raw `.text` of `mips_sample` in all six endian/ISA combinations it recovers the correct byte order 6/6, and its ISA lower bound never exceeds the declared `-march` (guard test). Negative controls: random data, text, zeros, and arm64 code from the desktop build are **not** reported as MIPS (an early version wrongly accepted the arm64 code; the decision rule now requires instruction-pattern anchors, because the MIPS32 encoding space is dense and "valid decode" alone proves nothing).

## 5. What additional evidence would distinguish the configurations
| Question | Decisive evidence | Obtainable without touching the camera? |
|---|---|---|
| Endianness | Instruction bytes of any original code (a firmware image): `jr $ra` is `08 00 e0 03` (LE) vs `03 e0 00 08` (BE) | Only if a firmware image is lawfully obtained |
| ISA revision | R2-only instruction presence in original code (toolkit reports a lower bound); or CPU Config register | Image analysis: yes if an image exists |
| Core family | Zoran/CSR documentation; or Config register `AR`/`MT`/PRId value | Documentation only |
| Instruction restrictions (e.g. no `lwl/lwr`) | Original code's use of those instructions | Image analysis |
| FPU | Presence of COP1 instructions in original code | Image analysis |

## 6. Decision
**No single binary is selected as the S650 target.** The experimental build keeps both byte orders and three ISA levels; `mips32` (R1, soft-float, o32, non-PIC) remains the default *working hypothesis* because it is the most conservative MIPS32 level consistent with a 2002 4KE licence, while `mips1` stays as an extra-conservative fallback.
