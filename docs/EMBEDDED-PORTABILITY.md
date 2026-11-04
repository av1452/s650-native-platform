# Embedded portability: measured dependencies and the route to a freestanding UI core (round 9)

Replaces the earlier estimate in `PORTABILITY.md` with **measurements** (tools: `tools/portability/measure.py`; raw output `docs/generated/portability-measure.md`). Method: for each application source, scan its standard-library use (direct and through project headers) and probe-compile it for the experimental target (`clang --target=mipsel-none-elf -march=mips32 -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -nostdinc++`), first with no standard headers at all and then with minimal shims (`target/shim`: cstdint, cstddef, cstring, cmath, algorithm map to compiler builtins). Compile-only; nothing here runs on any target.

## 1. What actually blocks compilation
Facility usage across the 29 application sources (files affected including via project headers / direct uses in `.cpp` files):

| Facility | Files | Direct uses | Replacement difficulty |
|---|---|---|---|
| `std::string` | 24 | 102 | Medium: mostly labels and formatting. A fixed-capacity string (`FixedString<N>`) and `string_view`-style references cover most |
| `std::vector` | 20 | 34 | Medium: pixel buffers, menu rows, options, images list. Mostly fixed upper bounds (8 categories, 12 setup rows, 16 images/page, 32 histogram bins) |
| `std::pair`/`tie` | 20 | 0 (via headers) | Trivial |
| `std::min/max/clamp/sort/swap` | 16 | 92 | Trivial: 20 lines of templates (shim exists) |
| `snprintf`, `std::stol`, `to_string` | 12 | 59 | Small: integer-to-text helpers; settings parse |
| `std::shared_ptr/unique_ptr` | 10 | 6 | Small: image previews and screens; use a pool / static objects |
| `std::function` | 8 | 10 | Small-medium: menu row callbacks -> plain function pointers or a tagged-union row model |
| math (`sin, cos, pow, log2, hypot, ...`) | 5 | 31 | Linking needs compiler-rt + a libm; FPU absent -> soft-float; drawing code could move to fixed-point |
| `deque`, `unordered_map`, `sstream/fstream`, `chrono` | 1-2 each | 5 | Desktop-only (input queue, font table, storage/settings files, system clock) |
| exceptions | 1 | 2 | `std::stol` try/catch in settings parsing (desktop file I/O) |

**Result before any change: 0 of 24 sources compiled freestanding, even with shims.** `Types.h` alone pulled in `<string>`, so every file failed at its first include.

## 2. First step taken (measured, behaviour-preserving)
Header splits so the *core data* needs no C++ standard library:
| Change | Reason |
|---|---|
| `core/Types.h`: `<stdint.h>` only | it included `<string>` without using it |
| `core/Options.h`: tables only; `evLabel` -> `core/OptionsText.h/.cpp` | text formatting is a separate concern |
| `core/Settings.h`: data + effective-value logic only; `serialize/deserialize` -> `core/SettingsIO.h/.cpp` | persistence is a platform service |
| `core/Clock.h`: interface only; calendar helpers -> `core/DateTime.h/.cpp` | date text is a presentation concern |
**Outcome:** the desktop build still passes **all original 86 tests** (suite is now 93 tests, 5,217 checks including the new ones). Freestanding probe: **4 of 29 sources compile with minimal shims** (`Options.cpp`, `Settings.cpp`, `SystemController.cpp`, `Theme.cpp`; 2 with no shims at all). These are the power state machine, settings logic, option tables and themes: the core of "what the camera should do", already free of the standard library and the OS.

## 3. Greedy unblock plan (what to provide, in order, and how many files each step frees)
From the measurement: chrono -> 4/29, C formatting helpers -> 5/29, `std::string` -> 7/29, ... `std::vector` -> **18/29**, `shared_ptr/unique_ptr` -> 21/29, `std::function` -> 29/29. **Three facilities (string, vector, function) account for almost everything**, and the pixel/draw code (`Framebuffer`, `Canvas`, `Font`, `Icons`) hinges on `std::vector` (frame buffers) and `std::string` (text API).

## 4. Comparison of routes
| Route | What it takes | Pros | Cons / unknowns | Verdict |
|---|---|---|---|---|
| **1. Small freestanding C++ runtime of our own** (`FixedString`, `StaticVector`, `FnRef`, int formatting) | ~170 edit sites (102 string + 34 vector + 10 function + 6 pointer + helpers); a few hundred lines of new library | No external dependency; deterministic memory; no heap needed; works with any loader | Writing/testing our own containers (the desktop tests make this safe) | **Recommended implementation technique** |
| **2. Lightweight embedded C++ standard library** (libc++/libstdc++ built for bare metal with newlib/picolibc, or the Embedded Template Library) | A target sysroot: C library, `compiler-rt`/`libgcc`, allocator, startup | Familiar APIs | Needs the **target ABI and runtime to be known and runnable** (they are not); a third-party dependency (ETL is header-only and has fixed-capacity `etl::string/vector/delegate`; not introduced, would be explained first); soft-float libs | Defer until an execution environment exists |
| **3. Refactor selected components to fixed buffers** | Apply route 1 piecemeal, component by component, with the desktop tests as the guard | Incremental, always shippable | Slower start | **How route 1 is executed** |
| **4. Hybrid: freestanding UI core + platform services from the original firmware** | UI core + a C ABI table of platform operations (draw/blit, key events, file list/open, settings read/write, camera state, time) that the original firmware (or a shim) fills in | Fits the preferred architecture (our UI integrated with the original camera software); smallest runtime surface; the same table can be filled by the desktop backends | Needs knowledge of which original functions exist (blocked on the image) | **Target architecture** |

**Recommendation:** adopt route 4 as the architecture, implemented by routes 3/1. Do **not** adopt route 2 until the target runtime is known. Do not rewrite the application beyond what the measurements justify: the desktop implementation stays as the reference and keeps passing its tests.

## 5. Staged plan (each stage measurable with `measure.py`)
| Stage | Work | Measurable result |
|---|---|---|
| P0 (done) | Header splits; measurement tools; shims | 4/29 compile freestanding; 93 tests pass |
| P1 | `FixedString<N>` + integer/hex formatting; replace `std::string` in `Strings`, `DateTime`, `Hud`, widgets, screens | `std::string` count 102 -> 0 outside desktop-only files |
| P2 | `StaticVector<T, N>` for menu rows, options, histogram, image lists; replace `Framebuffer`'s `std::vector` with a caller-supplied fixed buffer | files needing `std::vector` 20 -> desktop-only |
| P3 | Replace `std::function` in `MenuModel`/`Dialog`/UI callbacks with function pointers + context or a tagged row model; replace `shared_ptr` image previews with handles into a pool | files needing function/shared_ptr -> 0 |
| P4 | Define the C-ABI `S650Platform` operations table; adapt the existing four backends to fill it; keep desktop backends | UI core compiles freestanding; unchanged desktop tests pass |
| P5 | Fixed-point drawing (optional) and soft-float audit | no libm/compiler-rt dependence beyond integer helpers |
| P6 | Link a freestanding UI-core object into a target test program (still emulator-only) and run the menu state machine on QEMU against a scripted input table | UI core executes on an emulated MIPS core (still **not** the camera) |

## 6. Memory and speed notes (desktop measurements, unchanged)
Six to eight 307 KB frames can be alive at once on the desktop; a port should use **one** frame (or draw straight to an OSD plane). The software alpha blends and bilinear blits are unoptimised; effects degrade to cuts or opaque bars if a target CPU is slow (`PORTABILITY.md`).

## 7. What is not claimed
No part of the application has been built into a camera-loadable image, linked for the target, or run on any real MIPS hardware. The 4/29 figure means *compiles* for a MIPS32 soft-float freestanding target, not *runs*.
