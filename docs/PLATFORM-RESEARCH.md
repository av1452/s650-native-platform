# Zoran COACH 9S platform research (round 7, 2026-11-03)

Research and documentation only. Nothing was done to the camera. This extends `~/projects/s650-research/` and changes none of its conclusions.

**Grades:** **E** = confirmed for the exact ZR36473; **F** = confirmed for the COACH 9 family only; **I** = inferred from related hardware (different Zoran generation or a different product) - weak evidence, never proof; **U** = unknown / no source found.
Another Zoran processor's documentation is **never** treated as proof of compatibility; "I" items are leads, not facts.

## Sources
| Source | Used for | Notes |
|---|---|---|
| Zoran COACH 9 product brief (ZR36470/2/3, 04/06), copy at pdf.jiepei.com | Family features | Two-page marketing brief; no electrical, boot, memory-map or software detail. Lists `ZR36473BGCG`. |
| Design & Reuse, MIPS/Zoran licence news (9 Aug 2002) | MIPS32 4KE family licensed for COACH and digital-TV processors | Product line, not specific to the 9S |
| COACH-V datasheet snippet (search result) | Older generation: program may live in the same NAND as the images, no boot ROM | **Different generation** → grade I |
| COACH-6 data book terminology (search result) | Meaning of COACHWARE and HCE | Data book is proprietary; contents not read beyond a snippet → grade I |
| Camera-wiki "Zoran" | Lists no COACH 9/9s camera | Names only one COACH 1.0 camera |
| Sony update-instruction pages for other models (DSC-T1, F828, H2) | Sony used Memory-Stick-based firmware updates on those models | Different platforms; says nothing about S650 → grade I at best |
| Earlier rounds: Zoran FY2007 10-K, iFixit photos, service manual, retailer pages | See `FINDINGS-S650.md` | |
| alldatasheet.com COACH10 page | **403 Forbidden; not read.** Recorded as inaccessible; no circumvention attempted. | |
| Searches for source code, SDK, firmware dumps, hacking projects for COACH/Zoran cameras | **Nothing found** (negative result) | |

## The 14 questions
| # | Question | Finding | Grade |
|---|---|---|---|
| 1 | Exact CPU architecture, MIPS core, ISA | "32-bit MIPS CPU" for the COACH 9 family; Zoran licensed MIPS32 4KE-family cores in 2002 for COACH products. **The core in the 9S, ISA revision, endianness, clocks, caches, MMU, FPU are not stated in any source read.** | F (family) / U |
| 2 | Compiler and cross-compilation | If MIPS32: GCC/LLVM generic targets exist. No Zoran toolchain, ABI, linker map or runtime has been located. | I / U |
| 3 | Executable formats | No source describes any. | U |
| 4 | Boot process and bootloader | Not in the brief. An older generation's datasheet says the program can be stored in the image NAND so no separate boot ROM is needed - a lead only. | I / U |
| 5 | RAM initialisation and memory layout | Family: 16-bit DDR-SDRAM interface, 64 Mbit-1 Gbit. Photographed S650 board (other unit): 256 Mbit DDR. Init sequence, map, and how the pipeline shares RAM: unknown. | F / P / U |
| 6 | NAND/NOR boot support, firmware organisation | Family: direct NAND or NOR interface. Photographed board has NAND (not confirmed on this unit). Organisation unknown. Arithmetic: raw NAND 33,554,432 B vs observed volume 25,263,616 B leaves ≈ 8.3 MB for everything else - consistent with sharing, not proof. | F / I / U |
| 7 | LCD controller and display initialisation | Family: programmable controller for multiple LCD vendors; TV encoder. Panel timing and init for the S650 unknown. | F / U |
| 8 | Button input hardware | No source. | U |
| 9 | Sensor, lens, AF, exposure, flash interfaces | Not documented; board-specific Sony design. | U |
| 10 | Graphics and image-decoding capabilities | Family: JPEG/Motion JPEG codec, OSD with transparency, picture-in-picture blending, playback zoom to 16× and rotation, digital zoom. Software access unknown. | F / U |
| 11 | Operating system or firmware framework | **No source states an OS for the 9S.** Zoran material describes "HCE" (embedded host control) and "COACHWARE" libraries supplied by Zoran that run on the embedded CPU (generation-level terminology). Linux, uClinux, an RTOS, or bare-metal are all unproven. | F (terms) / I / U |
| 12 | Can the OSD hardware be controlled independently? | Unknown. It exists at family level. Whether Sony's firmware owns it exclusively and whether an add-on could use it is unknown. | F / U |
| 13 | Can an application run alongside the original software? | Unknown. Requires knowing the firmware's structure, scheduler, memory use and extension points. | U |
| 14 | Must the original firmware be modified? | Unknown, but **almost certainly yes** for any code to run, unless a documented card-based loader exists (none identified). | I / U |

## Camera integration strategy (technical comparison)
| Approach | What it needs | Preserves photography? | Verdict on current evidence |
|---|---|---|---|
| **A. Replacement interface that calls original camera functions** | A way to start our code, the original firmware's entry points for settings, state, files and drawing, and its memory/scheduling behaviour | Yes, if the original functions stay intact | **Preferred design, but blocked**: needs the original image or documentation. The architecture here already assumes it (`CameraBackend` forwards, never re-implements). |
| **B. Application integrated into the original firmware (patch or hook)** | Firmware image, format, load map, integrity checks, injection point, recovery | Yes | Same prerequisites as A plus patching; effectively how A would be realised. |
| **C. Extension (add a module) to the firmware** | A module/plug-in mechanism or spare space plus a loader | Yes | No evidence a mechanism exists (**U**). |
| **D. Replacement application on Zoran's framework** | HCE/COACHWARE libraries, tools, board support, drivers | Only if Sony's camera-specific drivers can be reused | Not public (proprietary data book/SDK); **blocked**. |
| **E. Replace the entire firmware** | Complete boot, memory map, every driver (sensor, lens, flash, AF, power, USB, storage), tuning | Only by re-implementing it | **Not recommended**: no evidence the camera can be fully initialised independently. |

**Recommendation:** keep the UI as a separate module behind the four backends and pursue **A/B** only if evidence appears; do not plan for **E**. Preserving the original photography functions outranks independence from the original software.

## What a camera port would require (checklist)
1. The original firmware image or authoritative documentation (boot, map, format, APIs).
2. A verified read-only way to obtain it and a verified restore path, rehearsed on a donor camera.
3. Identification of the CPU core and ABI; a toolchain that reproduces it; a safe test method.
4. A mapped list of original functions for: drawing/overlay, button events, settings, file list/decode, camera state, power.
5. A minimal proof-of-concept (one static screen) on a donor, then integration, then the full interface.
Gated stages with stop conditions are in `S650-HARDWARE-IMPLEMENTATION-ROADMAP.md` (research project).

## Firmware acquisition and recovery (Phase 6 summary)
- **No** legitimate artifact for the original S650 firmware has been found; Sony support pages show no S650 update (user-reported; my own fetch was blocked). Other Sony models of the era used Memory-Stick-based updates; no such mechanism is documented for the S650.
- A visible-photo copy is not a firmware backup. A raw NAND read is not automatically a usable image (spare areas, ECC, bad blocks, layout, repeatability) and a successful read does not prove restorability.
- Prerequisites before any physical work: documentation or image analysis; a restore method demonstrated on a **donor** camera; qualified electronics help and appropriate equipment; a written recovery plan; explicit acceptance of risk. **No experiments on the only working camera.** Nothing in this project authorises hands-on work, and no instructions for opening the camera or handling its flash circuitry are given.

## Conclusion
The evidence is unchanged: platform documentation and the original software are unavailable, and no recovery path exists. A native application cannot yet be targeted at the S650. The most valuable independent work is what this project does: a portable, backend-separated interface and an honest list of what a port would have to supply.
