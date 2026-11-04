# Execution path investigation (round 9)

Question: how could our own code eventually execute on the S650? Each candidate is judged by **what has evidence, what is speculative, what is missing**, using only existing research, public documents and desktop analysis. **Any candidate that needs the camera (touching it, connecting it, opening it) is marked UNAVAILABLE under the current restrictions and left out of the ranking.**

Classes: **V** verified for the S650 / **F** COACH-family document / **O** other chips or models / **H** hypothesis / **U** unknown.

## Candidate A: a separately loaded application on the original firmware's framework
| | |
|---|---|
| Technically possible? | Only if the original firmware has a runtime loader, module table or plug-in registry. Nothing says it does. |
| Evidence | The family brief describes the HCE architecture as enabling "customer creation of DSC applications with minimal effort" and "backward compatibility with previous COACH software API set" (F). That wording describes a **build-time** SDK used by camera makers, not a runtime loader. COACH-V (older generation) mentions a DSP engine whose microcode is "downloadable" (O): a different mechanism (DSP firmware), not application loading. |
| Speculative | A module loader or resource-driven UI exists in Sony's build (H, no support). |
| Missing | The firmware image; its task/module structures; the UI framework's data model. |
| Software-only progress | Analysis toolkit ready; once an image exists, search for module tables, function-pointer registries and resource directories (`FIRMWARE-ANALYSIS.md`). |
| Verdict | **Unknown, low prior.** Answerable only by image analysis. |

## Candidate B: integrate our interface into the original firmware (patch / hook)
| | |
|---|---|
| Technically possible? | Yes in principle for any firmware, given the image, a safe write path and a validated restore. This is the only candidate that keeps the original photography functions by construction. |
| Evidence | "Open firmware architecture for easy customer modification" and HCE (F) show the software is organised for customer-level modification *by licensees with the SDK*; nothing shows how to modify a shipped S650 image. Servicing is replacement-only (V). |
| Speculative | UI code is separable from the camera pipeline (H); a text/graphics layer can be hooked. |
| Missing | Image, format, load map, integrity checks (checksums/signatures), recovery. All blocked (`RECOVERY-REQUIREMENTS.md`). |
| Software-only progress | Embedded-portable UI core (`EMBEDDED-PORTABILITY.md`); a C-ABI platform-operations table so the UI can call original routines; reproducible target toolchain with both byte orders; ISA probe. |
| Verdict | **The route most likely to succeed if an image is ever obtained.** Requires approved read-only acquisition first. |

## Candidate C: boot custom code from a boot medium
| | |
|---|---|
| Possible? | Only if the boot ROM supports another medium or a strap-selected mode. |
| Evidence | Family: "direct interface to resident flash memory (NAND or NOR type)"; older COACH-V datasheet: program can sit in the image NAND, no ROM device needed (O). **No document mentions booting from a card or USB for any COACH part.** Zoran also sold a USB host/PC-connected camera line (zr364xx webcam-mode driver), which concerns image transfer, not code loading. |
| Speculative | A hidden boot-strap mode exists (H, no evidence). We will **not** invent boot modes or pins. |
| Missing | Boot-ROM description; strap pins (the service manual shows no `BOOT` net). |
| Verdict | **Unknown; testable only on hardware** -> UNAVAILABLE under current restrictions. |

## Candidate D: removable media (Memory Stick)
| | |
|---|---|
| Possible? | Only through a firmware-implemented mechanism such as an update-from-card feature. |
| Evidence | Sony's own pages for other Cyber-shot models (DSC-T1, F828, H2, and the W630/W650 update which is a *different camera*) describe firmware updates delivered on Memory Stick with a Sony procedure (O). Search results for the S650 list **no firmware update**; Sony Canada's S650 support page reportedly lists downloads dated 2007-2019 but **the page returns 403 to my tools** (title list unread, not worked around). A legacy Sony Europe support URL refuses connections. |
| Speculative | An S650 update-from-card path exists (H); it would be a *signed or checksummed update*, not a generic code loader. |
| Missing | The S650 download list. **You can check it legitimately in your own browser** (Sony Canada page for DSC-S650, "Downloads", and the equivalent pages for other regions) and tell me the item titles/dates; I will not scrape or circumvent. |
| Verdict | **No evidence of execution from media.** Worth one manual look at Sony's S650 download list. |

## Candidate E: USB
Known (V, host-side observation of the user's unit): Mass Storage `054c:0295` (INQUIRY vendor "SONY", FAT12 volume 25,263,616 bytes), PictBridge/PTP `054c:0296`, full-speed, **no vendor-specific interface**, PTP requests from macOS timed out.
| Avenue | Legitimate now? | Assessment |
|---|---|---|
| Re-read the saved descriptors/ioreg dumps in `~/projects/s650-research/` | Yes (offline) | Done in earlier rounds; no loader-like interface |
| Study the PTP time-outs | Yes (offline analysis of saved logs) | A host-stack/handshake issue is the likeliest explanation; it is not evidence of a loader |
| Capture standard Sony-software-to-camera traffic | **No**: needs the camera connected | UNAVAILABLE |
| Send vendor PTP/SCSI commands | **No**: forbidden | UNAVAILABLE |
| Analyse Sony's PC software (PMB/Cyber-shot Viewer) for camera protocols | Requires downloading unknown installers and Sony pages are blocked | Not done; not recommended |
| Verdict | **No avenue found that does not touch the camera.** USB remains a data channel (documented protocols only). |

## Ranking and what deserves investigation next
| Rank | Candidate | Why |
|---|---|---|
| 1 | **B (integrate into original firmware)** with **A** resolved as a by-product of the same image analysis | Only route preserving photography; every answer needed (loader? integrity checks? UI separability?) lives in the image |
| 2 | D (check Sony's S650 download list manually) | Zero hardware risk; could reveal a package whose *format* we can analyse offline |
| 3 | C | Needs hardware |
| 4 | E | Nothing to gain without touching the camera |

**What is technically possible vs evidenced vs speculative:** *possible* in principle: B; *evidenced*: only the family's "open firmware" intent and the NAND/NOR boot capability (F/O); *speculative*: loaders, boot straps, card execution. **Missing information (all of it solvable only by obtaining a lawful image or documentation):** boot path, firmware format, module system, integrity checks.

## Software-only work that advances these paths (done or ready)
1. `tools/firmware-analysis/` - ready to analyse any image/update package the moment one exists (format recognition, endianness/ISA inference, strings, comparison).
2. `isa_probe` + `qemu-isa-matrix.py` - a prepared experiment that would identify the core's ISA level if code could ever be run.
3. Target toolchain with both byte orders and a payload that needs no stack/runtime.
4. Embedded-portable UI core in progress (`EMBEDDED-PORTABILITY.md`) and a platform-operations seam for hooking into original routines.
5. LCD test pattern and raw-framebuffer backend (`LCD-RENDERING-RESEARCH.md`).
