# Target architecture: what is established about the ZR36473 CPU (round 8, 2026-11-03)

Research and build work only. The camera was not touched, opened, probed, or connected.

**Bottom line:** the processor family is known to be "32-bit MIPS" (COACH 9 family brief); **nothing more about the CPU is confirmed for the ZR36473.** The core, ISA revision, endianness, ABI expectations, FPU, MMU, memory map and startup behaviour are all **unestablished**. The cross-compiler settings in this project are therefore *labelled experimental hypotheses*, and both byte orders and three ISA levels are built so that no arbitrary choice is hidden.

Grades: **E** = confirmed for the ZR36473; **F** = confirmed for the COACH 9 family only; **I** = inferred from related hardware or from general MIPS documentation (a lead, never proof); **U** = unknown. A generic MIPS fact is never treated as a ZR36473 fact.

## The 12 questions
| # | Question | Finding | Source | Grade |
|---|---|---|---|---|
| 1 | Exact CPU core | Not stated anywhere read. Zoran licensed the **MIPS32 4KE core family** in Aug 2002 for COACH camera and digital-TV processors, naming no variant (4KEc/4KEm/4KEp) and no COACH generation. The COACH 9 family brief (04/06) says only "32-bit MIPS CPU". Whether the 9S core is a 4KE-family part is **not stated**. | Design & Reuse (9 Aug 2002), EE Times licence article, COACH 9 brief | F (MIPS32 CPU) / I (4KE) / U (core) |
| 2 | ISA version and extensions | Unknown. *If* a 4KE-class core: MIPS32 (4KE parts are Release 2 designs, MIPS16e). Not established for the 9S. | public MIPS core descriptions via search; datasheet not retrievable (blocked) | I / U |
| 3 | Endianness | **Unknown.** MIPS cores are bi-endian, selected by hardware configuration; no source gives the COACH 9S choice. Both are built. | general MIPS core documentation (not re-read this session) | U |
| 4 | Register architecture | Standard 32 GPR MIPS32 *if* MIPS32-compliant. Not confirmed for the 9S. | general MIPS | I |
| 5 | ABI requirements | Unknown. No toolchain, runtime or firmware has been seen. o32 is the conventional 32-bit MIPS ABI and what is built. | - | U (o32 = convention only) |
| 6 | RAM and memory constraints | Family: 16-bit DDR-SDRAM interface, 64 Mbit-1 Gbit. A **photographed other-unit** S650 board shows a 256 Mbit (32 MB) DDR part (identification unconfirmed on this unit). How the pipeline shares it, and how much is free: unknown. | COACH 9 brief, iFixit photos | F / P / U |
| 7 | Processor startup behaviour | Unknown. A standard MIPS32 core starts at the architectural reset vector (0xBFC00000, uncached kseg1) with `Status.BEV=1` and uninitialised caches; **an SoC may map a boot ROM or NAND window there, or use a different mechanism**. Not documented for the 9S. | general MIPS32 architecture (not re-read this session) | I / U |
| 8 | MMU | Unknown. 4KEc-class cores have a TLB; 4KEm/4KEp have simpler fixed or block mappings. Not established. | general MIPS core descriptions | I / U |
| 9 | Floating-point hardware | Unknown. 4KE-class cores have no FPU; unverified for the 9S. Code is built soft-float. | general MIPS core descriptions | I / U |
| 10 | Execute from NAND, NOR or RAM | Unknown. Family brief: direct NAND **or** NOR interface; an older generation's datasheet says the program can sit in the image NAND with no separate boot ROM (a lead only). | COACH 9 brief; COACH-V snippet | F / I / U |
| 11 | Bootloader or proprietary startup | Unknown. No firmware, update package, or boot description has been located. | all rounds | U |
| 12 | Recognisable executable format in the firmware | Unknown; no firmware image available. | - | U |

## Hypotheses used by the experimental toolchain
Chosen to be the **most conservative code generation that does not pretend to know more than we do**.

| Setting | Value built | Why (and why it is not a claim) |
|---|---|---|
| Endianness | **both**: `mipsel` and `mips` | Unknown. No default is declared correct. (`little` is the CMake default only because a default is needed.) |
| ISA | **three**: `mips1`, `mips32`, `mips32r2` | `mips1` runs on any MIPS core (most conservative); `mips32` and `mips32r2` are the hypotheses *if* the core is a 4KE-class MIPS32 part. |
| ABI | o32, soft-float, non-PIC (`-mno-abicalls -fno-pic -G0`) | Conventional 32-bit MIPS bare-metal choices; no dynamic loader, GOT or `$gp` setup is known to exist. |
| Runtime | none: no stack, heap, libc, exceptions, RTTI | Nothing is known to exist. |
| Link address | `0x80000000` placeholder (architectural kseg0 start); `0x80100000` for the emulator-only build | **Arbitrary.** Not camera memory. |

## What would turn hypotheses into facts
| Unknown | Evidence that resolves it |
|---|---|
| Core, ISA, endianness, FPU, MMU | A Zoran/CSR programmer's guide or data book for the COACH 9/9S, or analysis of the original firmware (instruction set and byte order are visible in the code), or a CPU `Config` register value read by running code on the chip |
| Startup and memory map | Boot description, or the firmware's early-boot code, or documented test/boot pins on a board of this type |
| Executable format, load address | The firmware image or a documented loader |

## Emulation model
QEMU 11.0.2 provides generic MIPS32 CPU models (4Kc, 4KEc, 4KEm, 24Kc, ...) and a `malta` board. They are used **only** to check that our toolchain output is valid MIPS for those models. They are not models of the ZR36473 or the camera; no ZR36473 model exists.
