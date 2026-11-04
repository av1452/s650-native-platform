# Development roadmap, status and final assessment

Date: 2026-11-03. Everything below is about the **desktop simulation**. Nothing was implemented, tested or run on the Sony DSC-S650.

## 1. What was implemented (desktop only)
| Area | Implemented | Remains simulated / missing |
|---|---|---|
| Architecture | Four backend interfaces + Clock; `Application`, `SystemController`, `UIController` | No real backend of any kind exists |
| Rendering | Software framebuffer, AA lines/circles, polygons, alpha, clip, bilinear blit, 5×7 proportional bitmap font, vector icons, PNG writer | No accented glyphs; not optimised; 32-bit pixels |
| Shooting screen | Overlay (mode, size/quality, flash, images left, battery, date/time, zoom bar, EV/WB/ISO chips, shutter/aperture readout, histogram, three grids, AF frames, countdown, toast), DISP cycle, self-timer, movie recording state, red-eye pre-flash timing, lens-settle animation | Live view, exposure numbers, capture, battery are **simulated**; `SIM` tag shown |
| Menu | 8 categories, pickers, sliders, date/time editor, info views, disabled-with-reason rows, remembered position, 3 themes, Setup (12 rows), Initialize/Format dialogs | Language: English only. USB row is a choice only. Format affects simulated storage only |
| Display options | 13 items, presets + Custom, live scaled preview, appearance switch | - |
| Playback | 4×3 index, viewer, info levels, zoom ×2/×4 with minimap, playback menu, slideshow with cross-fade, protect, delete one/all with confirmation | Images are procedural previews, not JPEG files |
| Power | Startup animation, shutdown, off state, auto power-off, startup toggle | Simulated lifecycle; no sound |
| Persistence | Settings serialised to a file; corrupt/missing/unwritable handled | Not tied to any real storage |
| Failure paths | `UnavailableCamera` (all calls fail visibly), memory-full, protected-delete, unwritable settings | - |
| Window | macOS Cocoa window + keyboard | Launch smoke-tested only |

## 2. What was tested and the results
Command: `./build/s650_tests` (also `ctest --test-dir build`).
- **Final run: 86 tests, 433 checks, 0 failures** (Release, Apple clang 17.0.0, macOS arm64, CMake 4.4).
- Coverage: settings (round trip, clamping, garbage input, persistence, unwritable path, corrupt file), menu navigation and all level transitions in modern and classic, Auto-mode locking, picker/slider apply-cancel-restore, disabled-row explanations, date/time editor, Initialize/Format dialogs and their defaults, display-options Custom state, playback navigation/zoom/protect/delete/delete-all/slideshow/repeat, shooting AF seek-lock, capture, self-timer, cancel, movie start/stop, memory full, missing camera, red-eye timing, power sequence, skip, auto-off (including activity reset), rendering (primitives, font metrics, PNG signature, determinism, theme pixels, dialog dimming, brightness, smoke pass of every screen × theme), simulated and unavailable camera, storage, key mapping, scripted input, clock helpers.
- **Screenshots:** `docs/screenshots/*.png` (20 frames from `--tour`), all inspected by eye.
- **Sanitizer build (address + undefined behaviour, RelWithDebInfo):** **UndefinedBehaviorSanitizer** (`-fsanitize=undefined,float-cast-overflow,implicit-conversion -fno-sanitize-recover=all`, RelWithDebInfo): the full suite passes with no reports (86 tests, 433 checks). **AddressSanitizer could not be used**: even a trivial "hello world" built with `-fsanitize=address` hangs in this environment, so memory-error checking is **not done** - it remains open.
- Not tested: AddressSanitizer / memory-error checking (tool hangs here); any non-macOS platform; a non-Clang compiler; real-time performance; the window backend's pixels and keyboard.

## 3. What failed or went wrong along the way (kept for honesty)
1. **First screenshot tour showed black bars** at the right and bottom of the live view: scenes drawn for 320×240 were placed in a larger buffer for drift/zoom. Found by looking at screenshots, **not** by a test. Fixed; a regression test now exists and was verified to fail when the bug is reintroduced. (An earlier attempt to verify that used a stale build and wrongly appeared to pass; redone properly.)
2. The tour script pressed the wrong number of Down keys and changed the grid instead of the appearance. Script error, fixed.
3. **Two of my own new tests had wrong expectations** (moving up from index 3 clamps to 0 as in the browser version; the auto power-off test advanced through the whole 1.35 s shutdown). Both were test bugs; the product behaviour matched the specification. Fixed in the tests.
4. The suite took 73 s because a test simulated 20 minutes at 60 fps; long waits now use 100 ms frames (13 s total).
5. The window backend could not be visually verified in this session.
6. **Found by looking at the `--no-camera` screenshots (not by the tests that existed):** with no camera backend the AF frames still turned green and "locked" on a half-press, and the Camera status view headed its section "Simulated camera" while the backend was unavailable. Both fixed (focus is a camera function and now stays idle without a camera; the heading says "Camera"), and a test covers the focus case.
7. AddressSanitizer hangs on any program in this environment (see above); the memory-safety check is therefore outstanding.

## 4. What is known about the real camera / still unknown
Known (documented or family-level): see `PLATFORM-RESEARCH.md` and `HARDWARE-REQUIREMENTS.md`. In short: a 32-bit MIPS CPU for the COACH 9 family, NAND/NOR support, an LCD controller and OSD with transparency at family level, a 2.0-inch panel documented as 480×240 dots, JPEG/Motion JPEG, Memory Stick Duo.
**Unknown:** exact core and ISA, operating environment, executable format, boot sequence, memory map, display/OSD programming, button interface, all camera function APIs, firmware location and integrity checks, any recovery method, and the original firmware itself.

## 5. Gated next stages
Software (can proceed now):
1. Port the remaining Version 2.0 gaps: accented glyphs (French/German), sounds, richer icons/typography, scenes. Reduce frame buffers, remove per-frame allocations, profile. Add a pixel-format layer (RGB565/palette) and a coordinate mapping layer for a different panel grid.
2. Try a freestanding build of the core library to learn which STL features must go (no target needed).
3. Verify the window backend visually and interactively on a machine with display access.

Hardware-adjacent (**blocked on evidence; no physical action authorised**): follow the ten gated stages in `~/projects/s650-research/S650-HARDWARE-IMPLEMENTATION-ROADMAP.md`. In order: obtain evidence lawfully (Sony support, platform documentation), decide on donor cameras, then only if firmware or documentation is obtained proceed to analysis, recovery validation on a donor, and a minimal proof of concept. No stage starts because an earlier one produced something interesting.

## 6. Final assessment (Phase 7)
**Does the native application have a plausible path to the S650?**
- **Software side: yes, it is real and tested.** The interface is complete enough on the desktop to reproduce Version 2.0 faithfully, and it is cleanly separated from hardware by four backends. That is the right shape for a port, and the work is reusable no matter how the hardware question resolves.
- **Hardware side: no path is established.** Nothing shows the camera can run our code. Blockers, in order of importance:
  1. **No original firmware or platform documentation** (boot, memory map, executable format, drivers). Only a two-page family brief exists publicly.
  2. **No verified recovery path** and no donor camera; any experiment on the only working camera is unacceptable.
  3. **No known way to start our code** (no loader, no update mechanism identified for the S650).
  4. **Camera functions are Sony-specific** (sensor, lens, AF, flash, power); the design therefore relies on the original firmware keeping them, which is only possible by patching/hooking the original software - again blocked by item 1.
  5. **Display and OSD programming, and button reading, are unknown.**
- **Recommended architecture if the evidence ever appears:** a replacement interface that calls the original camera functions (integrated with or hooked into the original firmware). Replacing the whole firmware is not recommended.
- **Missing breakthrough:** the original firmware (or authoritative platform documentation) **plus** a recovery method proven on a donor camera.
- **Recommended next step:** the non-hardware actions listed in the research project (written enquiry to Sony support; enquiry about COACH 9S documentation to the successor of Zoran; the user's decision on a donor camera), while this project continues to improve the interface and its portability. If those return nothing, the interface remains a desktop product and a design reference, and a companion viewer app for photos copied off the camera is the realistic deliverable.

**The final objective (running on the actual camera) has not been achieved and is not claimed.**


---

# Round 8 (2026-11-03): target execution path (build and research only)

Goal of this round: start a real route to executing our software on the DSC-S650. **Result: a verified cross-compilation environment and first target binaries exist, but no execution path on the camera has been found. Nothing has run on the camera.**

## Implemented
| Item | Status |
|---|---|
| Toolchain: Homebrew LLVM/Clang 23.1.4 + lld 23.1.4 (+ existing QEMU 11.0.2) | Installed, **verified** (MIPS and MIPSEL backends, objdump/readelf/size/nm/objcopy) |
| `cmake/s650-target.cmake`, `target/CMakeLists.txt`, placeholder linker script | **Built and used**; all settings labelled hypotheses |
| `target/tests/hello_s650.cpp` | **12 variants** build (2 byte orders × 3 ISA levels × 2 link addresses); 1,024-byte ELF, 168-byte flat binary; no stack, calls, libc, undefined symbols |
| `tools/target-build/{build,inspect,port-matrix}.sh`, `qemu-verify.py` | Working; reproducible build record per variant |
| Emulator check | **6/6 pass** on QEMU `malta` + 4KEc (both byte orders, all ISA levels); negative controls behave (paused CPU reads zeros; wrong endianness refused) |
| Desktop build and tests | **Unchanged**: still 86 tests / 433 checks pass |
| Docs | `TARGET-ARCHITECTURE`, `CROSS-COMPILATION`, `EXECUTION-ENVIRONMENT`, `FIRMWARE-ANALYSIS`, `DISPLAY-INTEGRATION`, `RECOVERY-REQUIREMENTS` |

## What failed or was unavailable
1. First configure failed on `-mno-shared` (not a clang flag; removed).
2. The bare-metal build has no `<cstdint>`; `hello_s650.cpp` now uses its own checked 32-bit type.
3. The macOS clang driver injected Mach-O linker arguments; the toolchain file now links with `ld.lld` directly.
4. lld's default 64 KB page alignment inflated the ELF to 66 KB; `--nmagic` reduced it to 1,024 bytes.
5. My first attempt to install LLVM in the background was killed with its shell; reran properly.
6. **Application portability: 0 of 24 sources compile for the target without a standard library** (first blockers `<cstdint>`, `<string>`, `<memory>`).
7. Two web sources for MIPS core documents were blocked or unreachable (an anti-bot "Access Denied" page and a DNS failure); recorded, not circumvented. Core-level statements are therefore general MIPS knowledge, graded as inference.

## Still unknown (the blocking facts)
Exact core/ISA/endianness/FPU/MMU of the ZR36473; startup mechanism and memory map; executable format and any loader; display controller programming; button reading; camera-function entry points; firmware location, format and integrity checks; and any recovery method.

## First viable hardware path, and the next concrete experiment
There is **no viable on-camera execution path today.** The first path that could become viable is: **(1) obtain the original firmware or documentation → (2) establish a validated read-only acquisition and restore on a donor → (3) analyse to find the loader/hook and display routines → (4) run `hello_s650` equivalent in that mechanism and show a test pattern.** Steps 1-3 gate step 4.

The next experiments, in the order I recommend. **Each item below marked "approval" must NOT start without your explicit approval:**
| # | Experiment | Purpose | Prerequisites | Risks | Safer alternative | Approval? |
|---|---|---|---|---|---|---|
| E1 | Written enquiries: Sony support (does S650 firmware/service software exist?), CSR/Qualcomm (COACH 9S documentation) | Could supply the missing documents at zero hardware risk | none | none | - | no |
| E2 | Visual identification of the board's ICs by a **qualified technician** (camera opened by them; photographs only; **no probing, no power**) | Confirm the processor marking suffix, NAND and RAM parts, spot any NOR, see whether debug pads exist | a qualified technician; a static-safe workspace; the user's written approval | physical damage; the flash circuit is hazardous (their expertise) | Use the iFixit photographs already analysed (other unit) | **yes** |
| E3 | Re-read the camera's USB descriptors (already documented) | Confirm modes and descriptors on this unit | none | none (standard enumeration only) | Already done earlier | only if you want it repeated |
| E4 | Read-only in-circuit or off-board NAND read, twice, hashed, with a qualified person | First image to analyse | all prerequisites in `RECOVERY-REQUIREMENTS.md` (1-4 at least), programmer, adapters, approval | possible damage; unusable dump without ECC/layout knowledge | Wait for E1 results | **yes** (not recommended yet) |

My recommendation is **E1 now (no approval needed), E2 only if you approve and have a qualified technician, and nothing beyond E2 until E1/E2 results justify it.** I will not proceed to E2-E4 without your explicit approval.


---

# Round 9 (2026-11-03): independent reverse engineering without touching the camera

The camera stayed closed and untouched; no USB command was sent; no storage was written. Everything below is software, documents and emulation.

## Implemented and verified
| Item | Result |
|---|---|
| Existing work preserved | Desktop build unchanged in behaviour: **all original 86 tests pass**; suite is now **93 tests / 5,217 checks** (7 new: pixel formats, raw framebuffer, test pattern). All 12 target variants, build records and QEMU results retained |
| `tools/firmware-analysis/` | 8 modules + CLI; **20 tests pass** (fixtures for 15 formats, false-positive controls on 4 MB of random/text data, foreign-architecture control, ground-truth byte order 6/6, ISA-bound guard) |
| MIPS target analysis | 3 programs x 12 variants compared; QEMU: `mips_sample` **24/24** (4 CPU models x 6 variants), `hello` 6/6, `isa_probe` across 18 CPU models per byte order (R1 cores stop at the first R2 instruction) |
| Portability | Header splits (`Types.h`, `Options.h`, `Settings.h`, `Clock.h`); freestanding probe **0 -> 4** compiling sources; measurement tool and shims |
| Display | `RawFramebufferDisplay` (RGB565/332/Gray8/Palette8/RGB888, stretch/center, word order, dirty rectangle); full app runs on a 480x240 RGB565 framebuffer unchanged; test pattern + reference files |
| Research | Qimonda DDR datasheet read; hardware profile consolidated; execution paths A-E; other-camera survey; firmware search with changed strategy |

## What failed or was unavailable (honest list)
1. My first toolkit MIPS detector **labelled arm64 code as MIPS** (foreign-architecture control caught it): redesigned around instruction-pattern anchors.
2. An ISA bound derived from `ehb` (a small-constant encoding) was a false "needs Release 2"; weak encodings are now excluded.
3. A `_hit()` keyword collision and a precedence bug in the gzip validator were found by the first test run and fixed.
4. Two web sources (Sony support/download pages, an anti-bot page) were blocked; one legacy Sony URL refused connection. Recorded, not circumvented. No S650 firmware or download list could be read.
5. The datasheet fetched is Rev. 1.2 (2004); the 2007 revision was not retrievable as text.
6. My first vexing-parse error in a test and my own mistaken test expectation for block-shift detection (documented as a toolkit limitation).
7. Everything requiring the camera is **unavailable** under the current restrictions.

## Stage plan toward running our code on the S650
Each stage lists required evidence, available resources, what software alone can do, what remains unknown, dependencies and a **measurable completion criterion**. **No stage beyond 2 is complete; none is marked complete on speculation.**

| # | Stage | Required evidence | Available now | Software-only work possible | Still unknown | Depends on | Completion criterion | Status |
|---|---|---|---|---|---|---|---|---|
| 1 | Identify the processor architecture | Core/ISA/endianness from documents or original code | COACH 9 brief (MIPS32, family); 2002 4KE licence; hypotheses and discriminating probe | Toolkit to read it from an image; `isa_probe`; both byte orders built | Core, ISA revision, endianness, FPU, MMU | - | Original code (or a Zoran document) shows byte order + ISA level, recorded with a toolkit report | **Not complete** (family level only) |
| 2 | Establish the compiler target | Result of stage 1 | LLVM/lld toolchain, 12 variants, verified on QEMU | Reduce to one variant once stage 1 answers; add a GCC cross-check | The correct variant | 1 | One variant remains; `-march` <= core's level; reproduced hash | **Experimental toolchain done; target not selected** |
| 3 | Understand the original runtime and boot | Boot description or early-boot code | Family statements only | Toolkit + Ghidra plan; boot-code signatures | Boot ROM, loader, RTOS/bare-metal | 1 | Documented boot sequence with addresses from evidence | Blocked (needs image/docs) |
| 4 | Establish the firmware structure | A lawful image or update package | Nothing | Toolkit ready; Sony S650 download list check by the user | Everything | 3 | Format, sections, compression, integrity checks documented from the image | Blocked |
| 5 | Determine how code can be loaded | Loader, module system, or patch point | Candidate analysis A-E | Payload design that needs no runtime | Existence of any loader | 3, 4 | A documented, reproducible mechanism, validated on emulation of the *documented* format | Blocked |
| 6 | Understand the display interface | Controller/OSD programming facts | OSD/GUI family statements; backend spec; raw framebuffer; test pattern | Pixel-format and overlay support in the renderer | Controller, format, plane ownership | 3, 4 | Documented format, size, and ownership of the display/OSD plane | Blocked (spec ready) |
| 7 | Build a minimal camera-compatible program | Stages 1-5 results | `hello_s650` (generic), probe | Package payload per the loader's format | Format, addresses, startup state | 1-5 | Binary matches the documented load format; static analysis passes the toolkit's format check | Blocked |
| 8 | Establish a safe, verified recovery method | Complete image + spare/ECC layout + restore rehearsal | Requirements document | Procedures, checklists | NAND layout, ECC, restore | 4 | Bit-exact restore proven on a donor unit by a qualified person | Blocked (physical, needs approval) |
| 9 | Execute a minimal test program on the camera | Stages 5, 7, 8 | none | Emulator rehearsal of the documented format | Everything on hardware | 5, 7, 8 | The program's marker is **observed** from the camera (e.g., on screen), after approval | **Not started; never claimed** |
| 10 | Display the test pattern | Stage 6 + 9 | Pattern + reference hash | Final backend | Display facts | 6, 9 | A photograph of the real LCD matches the reference pattern | Not started |
| 11 | Integrate the C++ interface | Stages 6, 9, 10 | Desktop UI; 4/29 sources freestanding | P1-P6 of `EMBEDDED-PORTABILITY.md` | Target runtime | 9, 10 | UI core runs on the camera against real input | Not started |
| 12 | Connect original camera functions | Entry points and calling conventions | none | C-ABI platform table | All functions | 4, 11 | Shooting, playback, settings drive the real functions; regression checklist passes | Not started |

## Next concrete steps (ranked)
1. **You** open Sony's DSC-S650 support/download pages in your own browser (Sony Canada page reportedly lists items from 2007-2019) and report the titles/dates. I cannot read them. No risk, may reveal an analysable package.
2. Written enquiries to Sony support and the successor of Zoran for documentation (draft on request).
3. Continue software-only work: P1-P3 of the portability plan, overlay-capable renderer, toolkit tuning on any real MIPS firmware that becomes available.
4. Any camera-touching step (E2 chip identification by a technician, E4 read-only NAND read) remains **unavailable** until you lift the restriction and approve it explicitly; see `RECOVERY-REQUIREMENTS.md`.
