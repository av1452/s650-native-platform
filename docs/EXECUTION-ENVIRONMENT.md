# How the S650 starts and runs software (round 8)

**Short answer: not established.** No source located describes the S650's boot, firmware structure, or any way to run additional code. This document answers each question with what is known, the source, a grade, and what would resolve it. Nothing was sent to or read from the camera this round. USB observations below come from the earlier, user-run, host-side enumeration in `FINDINGS-S650.md`; no new commands were sent.

Grades: **E** exact ZR36473 · **F** COACH 9 family · **I** inferred (general MIPS, other generation, other product; a lead only) · **U** unknown.

| # | Question | Answer | Grade |
|---|---|---|---|
| 1 | Does the camera use a bootloader? | Unknown. Typical SoCs use a boot ROM plus a loader; an older Zoran generation's datasheet says the program can live in the image NAND with no separate boot ROM. Nothing for the 9S. | I / U |
| 2 | Where is it stored? | Unknown. The photographed board has one NAND; the arithmetic (33,554,432 B raw vs 25,263,616 B observed volume leaves ≈ 8.3 MB) is consistent with firmware sharing it, **not proof**. The service manual shows an 8-bit NAND interface with R/B and control signals but no layout. | I / U |
| 3 | How does the CPU find its first instructions? | Unknown. A standard MIPS32 core fetches from 0xBFC00000 (uncached kseg1) after reset; an SoC may map boot ROM/NAND there or boot differently. Not documented for the 9S. | I / U |
| 4 | How is RAM initialised? | Unknown (DDR controller setup is chip-specific; not documented). | U |
| 5 | Kernel or proprietary runtime? | Unknown. The family brief mentions "HCE" (embedded host control) and an "open firmware architecture"; Zoran COACH material elsewhere describes COACHWARE libraries on the embedded CPU. No OS is stated. **Linux, an RTOS, or bare-metal are all unproven.** | F (terms) / U |
| 6 | Does it load applications from NAND? | Unknown. | U |
| 7 | Is there an application framework? | The family brief says HCE and "backward compatibility with previous COACH software API set". Whether Sony's S650 uses it and how: unknown. | F / U |
| 8 | Can the firmware load modules? | Unknown. No evidence of a plug-in mechanism. | U |
| 9 | Extension or plug-in architecture? | No evidence. | U |
| 10 | Can an application be launched from removable media? | **No evidence.** Other Sony models of the era (DSC-T1, F828, H2) used firmware updates delivered on Memory Stick with a Sony tool/file convention, but those are different platforms; no S650 update or card-based mechanism has been identified (Sony's regional pages show none, user-reported). A card update path, if one existed, would be a *firmware update*, not a generic code loader. | I / U |
| 11 | Executable container or proprietary format? | Unknown; no image. | U |
| 12 | Is the original UI a separate application or part of one image? | Unknown. | U |

## USB Mass Storage / PTP / PictBridge as a loading channel?
Observed earlier (host side, `FINDINGS-S650.md` §2): the S650 enumerates as `054c:0295` (Mass Storage, INQUIRY vendor "SONY", FAT12 volume 25,263,616 bytes) and `054c:0296` (PictBridge/PTP); **no vendor-specific (0xFF) interface**; the PTP request from macOS timed out; full-speed link. These are the standard protocols the COACH 9 brief lists. **Nothing in these interfaces provides an application-loading capability, and none should be assumed.** Possible future uses (each needs approval): observing the documented descriptors again is non-invasive; probing unknown commands is not permitted.
Mass Storage files are ordinary data files on the user volume; **the volume is not evidence of where firmware or settings live.**

## What the evidence says about *our* options
- **Run as a stand-alone application:** needs a loader and a known memory map; neither exists in any source.
- **Run beside the original firmware:** needs the firmware's scheduling and memory model; unknown.
- **Patch/extend the original firmware:** needs the image; unknown, and unrecoverable without a validated restore (see `RECOVERY-REQUIREMENTS.md`).
- **Card or USB loader:** no evidence one exists.
- **Hardware debug port (JTAG/EJTAG/UART):** the service manual lists none and shows no `ICE_*`/`JTAG`/`UART`/`BOOT`/`DBG` nets (absent from the extracted text). A MIPS core normally includes EJTAG logic, but whether it is bonded out on this chip or board is unknown. **No pads may be probed without approval.**

## What would resolve the startup question
| Evidence | How it could be obtained | Needs approval? |
|---|---|---|
| Boot description / memory map | Zoran/CSR documentation via formal enquiry (not yet made) | no |
| Firmware image | Sony (written enquiry); or a verified read of the spare unit's flash | enquiry no; chip read **yes** |
| Boot pins or loader behaviour | Documentation or analysis of the firmware's early boot code | analysis of an image: no; any bench work: **yes** |
| Core configuration (CPU Config register, endianness) | Reading it requires running code on the chip, or analysing firmware | **yes** (later) |
