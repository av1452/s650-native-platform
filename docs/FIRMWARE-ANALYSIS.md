# Original firmware analysis (round 8)

**Status: no firmware image exists to analyse.** Nothing about the S650's firmware format, boot split, compression, drivers or integrity checks is known from any source. This document records what has been checked, what is *not* assumed, and a ready analysis plan that applies only to a **legitimately obtained, read-only** image.

## What has been checked
| Possible source | Result |
|---|---|
| Sony S650 update/firmware package | None identified. Sony regional pages show no S650 firmware (user-reported); my own fetch of Sony pages was blocked (403) and is **not** worked around. |
| Sony service software / update tool for the S650 | None found. The S650 service manual contains no firmware-replacement or version-check procedure. |
| Zoran/CSR SDK, HCE or COACHWARE | Proprietary; only briefs and terminology found. |
| Firmware for other Zoran cameras | **Not used.** Compatibility with the S650 is not assumed. |
| Public projects (CHDK-like, Ghidra databases, repos) | Nothing for COACH 9/9S. (CHDK works through a Canon-specific mechanism.) |

## Not assumed
That the firmware is a single image; that it has a recognisable header; that it is compressed; that it contains an RTOS or Linux; that the UI is a separate module; that the boot code is separate from the application; that addresses or register definitions from any other chip apply. **No memory address or register definition is stated in this project as a fact about the ZR36473.**

## Questions the firmware would answer
| Topic | What to look for |
|---|---|
| Image format | Magic bytes, headers, length/CRC fields, section tables, an update-package wrapper vs raw flash contents |
| Boot / application separation | A small first-stage region, a second-stage loader, the application; their load addresses |
| Compression | High-entropy regions, known compressors (LZ-family) and their headers |
| Executable sections | Code vs data vs resources (fonts, bitmaps, strings); instruction set and **endianness** visible in the code itself |
| Memory initialisation / hardware init | Early-boot code writing a block of controller registers, DDR configuration |
| Drivers | LCD, buttons, NAND/Memory Stick, USB, CCD/AFE, lens, flash, power |
| Application lifecycle | Task/scheduler structures, main loop, message passing |
| Integrity checks | Checksums, CRCs or signatures verified at boot or on update |
| UI structure | How screens are defined: code, tables, or resources; the string table; the font and icon formats |
| Recovery | A fallback loader, a recovery mode, or write-protected boot region |

## Analysis plan (read-only; applies only once an image is obtained lawfully)
1. Hash the image (SHA-256) and keep the original untouched; work on copies.
2. Entropy and structure scan; locate strings; look for headers and file-system signatures.
3. Determine instruction set and endianness from the code density and typical prologues (MIPS: `addiu sp,sp,-N`, `jr ra`; compare LE vs BE).
4. Load into **Ghidra** (not installed yet): Raw Binary, language **MIPS:LE:32:default** or **MIPS:BE:32:default** (whichever the evidence supports), base address from the boot code's jumps; skip auto-analysis at first.
5. Identify the boot region, the vector layout, and hardware-initialisation blocks; document register accesses as *observed*, never as guessed names.
6. Locate the LCD, button, and file-system routines; record entry points as a table (address, behaviour, confidence).
7. Compare two units' images if available (differences reveal calibration and settings regions).
Tools on hand now: `llvm-objdump`, `llvm-readelf`, `llvm-mc` (MIPS-capable disassembler, both endiannesses), QEMU for validating small code snippets. Not installed: Ghidra, GDB, binwalk.

## NAND-dump caveats (if an image is ever read from the chip)
A raw NAND dump may contain spare areas with ECC and bad-block markers; the camera's controller may use a spare-area layout and ECC scheme not documented; a data-only read can be unusable for restoration; two independent reads must match; the image is **not** a recovery method until a restore is demonstrated on a donor unit. See `RECOVERY-REQUIREMENTS.md`.

## Conclusion
Firmware analysis is blocked on acquiring an image. Acquisition is blocked on an approved, qualified, read-only method and a validated recovery path, neither of which exists yet.
