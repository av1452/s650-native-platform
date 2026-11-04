# Portability: from desktop simulation to a constrained target

> **Superseded in part by `EMBEDDED-PORTABILITY.md` (round 9), which replaces the estimates below with measurements.** The principles here still hold.

**Nothing here has been built or run for any target other than a macOS desktop.** This document lists what is portable by design, what is not, and what would need to change. It does not claim any target is supported.

## Portable by design
- **No OS services in the interface.** Screens, menu model, HUD, theme, settings, power state machine and dialog use only the backend interfaces and the C++ standard library. No threads, no files, no sockets, no windowing in `ui/` or `core/` (file access is confined to `SimulatedStorage` and PNG output).
- **Single drawing surface.** All pixels pass through `Framebuffer`/`Canvas`; the final step is `DisplayBackend::present`.
- **Logical input only.** `Action` events; no pointer, no touch.
- **Time injected.** Everything uses `Clock`, so there is no hidden dependence on system time (tests use a manual clock).
- **Fixed logical resolution (320×240).** Layout constants are in `ui/Theme.h` and the screens; the porting spec lists them. A different real panel grid would need either a coordinate mapping layer or a re-layout (not implemented).
- **No exceptions or RTTI needed** except one `try/catch` around `std::stol` in settings parsing (easy to replace).

## Not portable as written (needs work for an embedded target)
| Area | Today | For a constrained target |
|---|---|---|
| C++ runtime | Uses `std::string`, `std::vector`, `std::unordered_map`, `std::function`, `std::shared_ptr`, `<sstream>`, `<fstream>` | Whether a C++ standard library is available is unknown. Core data structures would need review (fixed-size buffers, static string tables); `<sstream>`/`<fstream>` are only in settings/storage code. |
| Heap use | Allocations every frame in places (strings for labels, `Framebuffer` copies in preview/boot fade, `buildMenu` rebuilt per input/render) | Preallocate; cache menu text; avoid per-frame framebuffer copies. |
| Pixel format | 32-bit `0xAARRGGBB` | Likely a 16-bit or palette/YUV overlay on real hardware (unknown); a pixel-format layer in `Canvas` would be required. |
| Floating point | AA line/circle/blit use `float`; toy exposure model uses `float`/`std::pow` | If no FPU: fixed-point drawing; the exposure model is simulation-only and would be removed. |
| Text | UTF-8 decode + 5×7 bitmap font; ASCII + a few symbols | Acceptable; accents needed for French/German. |
| Memory | Several full frames alive at once | Reduce to one frame (or draw direct to the overlay) - the largest saving. |
| Speed | Software alpha blending and bilinear blits per pixel; no optimisation | Profile on target; degrade effects (cross-fades, translucency) to cuts/opaque bars first. |
| Endianness/word size | Not assumed in logic; PNG writer is byte-wise | Expected neutral, unverified. |
| Build system | CMake + host compiler | A cross toolchain for the unknown CPU variant (see `PLATFORM-RESEARCH.md`). |

## Rule for adding code
Anything that touches hardware, files, time, or the OS belongs behind a backend interface. Anything under `ui/` must stay free of them.

## Candidate cross-compilation facts (unverified for this camera)
- If the processor is a MIPS32 core as the family brief says, GCC and LLVM both have generic MIPS targets. That is **not** evidence that code built for them would run: the exact core revision, endianness, FPU presence, ABI, load address, startup code, and C runtime are unknown, and no boot or executable format has been identified.
- A first useful step, if documentation appears, would be to build the core library freestanding (`-ffreestanding -fno-exceptions -fno-rtti`) with a minimal allocator to see which STL pieces must go. That has not been tried.
