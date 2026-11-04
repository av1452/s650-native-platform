# Independent reverse-engineering programme: status and method (round 9, 2026-11-03)

**Objective (unchanged):** run our own redesigned interface directly on the DSC-S650, preserving its photography functions. **Not achieved; nothing has run on the camera.** This round advanced everything that can be advanced *without touching the camera*: hardware profile, MIPS configuration analysis, an analysis toolkit, execution-path analysis, the display path, and embedded portability of our C++ code. The camera was not opened, probed, connected to, or modified; no USB command was sent; no storage was written.

## 1. Evidence discipline (used in every document)
Verified camera facts (V) / family-level specifications (F) / other-camera evidence (O) / engineering hypotheses (H) / unknown (U). A chip marking from another unit is evidence **T** (teardown of another unit), never confirmation for the user's unit. `HARDWARE-PROFILE.md` applies this to every row.

## 2. What was done this round
| Area | Result | Document |
|---|---|---|
| Hardware profile | Consolidated processor/RAM/NAND/service-manual/USB facts; **Qimonda DDR datasheet read** (4 banks x 8192 x 512 x 16, DDR400B, VDD 2.6 V, 200-cycle DLL lock, init sequence); NAND geometry labelled as inferred from sibling parts; no new teardown photographs exist | `HARDWARE-PROFILE.md` |
| MIPS configuration | 12 variants compared; emulator matrix over every QEMU MIPS32 CPU model; Release-1 vs Release-2 discrimination via `isa_probe`; analysis of which flags can introduce incompatible instructions; **no variant selected** | `MIPS-ANALYSIS.md` |
| Firmware analysis toolkit | `tools/firmware-analysis/`: 8 modules, CLI, 20 passing tests incl. false-positive and foreign-architecture controls | `FIRMWARE-ANALYSIS-TOOLKIT.md` |
| Execution paths | A-E investigated individually; ranking; B (integration into the original firmware) is the only route preserving photography, blocked on an image | `EXECUTION-PATH-INVESTIGATION.md` |
| Display | Documents' OSD/GUI statements analysed; `RawFramebufferDisplay` + test pattern + backend specification | `LCD-RENDERING-RESEARCH.md` |
| Embedded portability | Measured dependencies; header splits; 0 -> 4 freestanding-compiling sources; staged plan | `EMBEDDED-PORTABILITY.md` |
| Desktop regression | 93 tests / 5,217 checks pass (original 86 + 7 new); toolkit 20 tests; QEMU checks 24/24 and 6/6 | `DEVELOPMENT-ROADMAP.md` |

## 3. Original firmware search (change of strategy from earlier rounds)
Earlier rounds searched Sony pages and general terms. This round varied the approach and recorded each dead end.
| Approach | Result |
|---|---|
| Sony Canada DSC-S650 support page | **403 Forbidden to my tools.** A search snippet says it lists downloads dated 2007-2019 (titles not shown). Not worked around. **You can open it in your own browser and tell me the item titles** (it is the one legitimate unresolved lead). |
| Sony Brazil download W0007467 (returned by search) | 403; unknown content (not shown to concern the S650) |
| Legacy Sony Europe S650 page (named in a Sony community reply) | Connection refused (site gone) |
| Other Sony models' firmware pages (DSC-T1, H5, W630/W650...) | Exist, but are **different cameras**; not applicable and not to be installed. They show Sony delivered updates for those models as downloadable packages |
| S650 service manual | No firmware/service-software procedure (mechanical revisions only) |
| `ZR36473` + teardown/PCB/firmware/dump/custom-firmware searches | Nothing beyond distributor listings and the Zoran briefs |
| GitHub/forums for COACH SDK/HCE/COACHWARE source | None. The COACH-6 data book is proprietary; briefs list "COACH SDK", "PC-HCE" sections only by title |
| CHDK-style projects for Zoran cameras | None; CHDK's technique is Canon-specific |
**Conclusion:** no original S650 firmware, update package, service software or Zoran S650 reference firmware has been located. If an artifact is ever found, the procedure is fixed: verify source, record filename/context, hash it, identify its format read-only with the toolkit, never execute an installer, never install anything on the camera.

## 4. Other cameras using COACH hardware
Purpose: transferable architecture information only. No firmware from another camera is claimed compatible.
| Camera / product | Processor evidence | Firmware / SDK availability | Notes |
|---|---|---|---|
| **Sony DSC-S650** (ours) | `ZR36473` = COACH 9S per SEC filing; marking seen on another unit | None found | - |
| **Pentax Optio A20** | Named as a **COACH 9-family** camera in Zoran's Jan 2007 DivX-certification release (a family statement; 9S not specified) | Owner-restricted update (earlier rounds); **not pursued** (not owned) | The only camera tied to COACH 9 by a source I found |
| **Zoran CamMini 9** | Reference camera "based on COACH 9", CCD and CMOS versions; ships with COACH firmware, **HCE-9 reference code** and WHQL drivers | Proprietary (licensees only) | Shows the intended software structure |
| Sony (generally) | Trade press (Electronic Design, Feb 2008) lists Sony among COACH customers; **no model or generation** | - | Does not tie the S650 to the 9S beyond the SEC filing/marking |
| Pentax (generally), Kodak, Nikon, Olympus, Samsung, others | Same article: makers using COACH single-chip processors; generation unspecified | - | Not specific |
| Minox Digital Classic Camera (Leica M3 replica) | Camera-wiki: COACH **1.0** | - | Different generation |
| Zoran "zr364xx" USB webcam-mode cameras | Linux kernel driver documentation: webcam mode for ZR364xx/COACH-based USB cameras | Open driver for the *USB webcam interface only* | Not firmware |
| Camera-wiki "Zoran" page | Lists no COACH 9/9S camera | - | - |
**No publicly documented reverse-engineering project or custom-firmware experiment** for COACH 9/9S was found. Transferable architectural information is limited to Zoran's own statements: HCE host layer, COACHWARE libraries, OSD/GUI framework, NAND/NOR program storage in older generations.

## 5. Dead ends and lessons
- Decode validity is not an architecture detector for MIPS (arm64 code decoded "validly"); instruction-pattern anchors are required. Fixed in the toolkit and covered by a test.
- `hello_s650` cannot discriminate hypotheses (identical across 12 variants); `mips_sample` and `isa_probe` can.
- Homebrew's libc++ is configured for Apple platforms; a bare-metal C++ library must be built for the target, which needs the (unknown) runtime.
- AddressSanitizer hangs in this environment (earlier round); only UBSan was usable for the desktop code.
- Web sources blocked (Sony pages 403, one anti-bot page) are recorded, not circumvented.

## 6. What is blocked by the restrictions (marked unavailable)
Anything requiring the camera: connecting it for traffic capture, entering modes, reading memory, running `isa_probe`/`hello_s650` on it, identifying chip markings on the user's own unit, any storage read. These stay unavailable until the restrictions change and the prerequisites in `RECOVERY-REQUIREMENTS.md` are met.
