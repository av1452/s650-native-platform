# Cross-compilation environment (experimental)

**What this is:** a reproducible build of a bare-metal MIPS test program for a *hypothetical* ZR36473 configuration (see `TARGET-ARCHITECTURE.md`). **A successfully compiled MIPS executable is not proof that the camera can execute it.** The ELF files here are generic bare-metal MIPS ELFs; nothing has been run on the camera, and nothing is known about how the camera could load them.

## Toolchain decisions
| Tool | Decision | Reason |
|---|---|---|
| **LLVM/Clang 23.1.4 (Homebrew)** | **Used** | One install covers compiler, assembler, `llvm-objdump`, `llvm-readelf`, `llvm-nm`, `llvm-size`, `llvm-objcopy`, and both MIPS endiannesses (`mips`, `mipsel`) via `--target`. Apple's own clang has no MIPS backend. |
| **lld 23.1.4 (Homebrew)** | **Used** | ELF linker for the bare-metal link; invoked directly because the macOS clang driver injects Mach-O linker arguments. |
| GCC cross compilers (`mips-linux-gnu-gcc`, `mips-elf-gcc`) | Not installed | Not needed yet; would be a second opinion on codegen. Would require a third-party Homebrew tap or a source build. |
| GNU binutils / GDB | Not installed | `llvm-*` tools cover inspection. GDB would matter once there is something to debug on real or emulated hardware with a gdbstub. |
| **QEMU 11.0.2** | **Used (already installed)** | Independent MIPS implementation to validate toolchain output on generic cores 4KEc (MIPS32). |
| Ghidra | Not installed | Nothing to analyse yet (no firmware). See `FIRMWARE-ANALYSIS.md` for the planned import settings. |

Dependencies added by this phase: `brew install llvm lld` (about 1 GB; Homebrew formulae `llvm` 23.1.4, `lld` 23.1.4, plus `z3`). QEMU and CMake were already present. The desktop build needs none of them.

## Layout
```
cmake/s650-target.cmake     toolchain file (experimental; every setting labelled as a hypothesis)
target/CMakeLists.txt       separate project for the target; does not touch the desktop build
target/linker/placeholder.ld   PLACEHOLDER linker script (arbitrary link address)
target/tests/hello_s650.cpp    first target program
tools/target-build/build.sh    builds every endian x ISA x link-address variant, writes build-info + inspection
tools/target-build/inspect.sh  static inspection of one ELF
tools/target-build/qemu-verify.py  runs the qemu-address variants on an EMULATED 4KEc and reads the test buffer
tools/target-build/port-matrix.sh  which application sources compile with no standard library
build-target/               generated output (not committed)
```
The desktop CMake project (`CMakeLists.txt`, `build/`) is unchanged; `build-target/` is a separate directory.

## Reproduce
```sh
brew install llvm lld                 # once
tools/target-build/build.sh           # 12 variants -> build-target/*/hello_s650.{elf,bin,map}, inspection.txt, build-info.txt
python3 tools/target-build/qemu-verify.py   # emulator check (6 qemu variants)
tools/target-build/port-matrix.sh     # application portability matrix
# single variant:
cmake -S target -B build-target/one -DCMAKE_TOOLCHAIN_FILE=cmake/s650-target.cmake -DS650_ENDIAN=little -DS650_ISA=mips32
cmake --build build-target/one
```
`build-info.txt` in each output directory records the compiler version, the link address and the exact compile command (from `compile_commands.json`).

## Flags (all in `cmake/s650-target.cmake`)
`--target=mipsel-none-elf | mips-none-elf`, `-march=mips1|mips32|mips32r2`, `-mabi=32 -msoft-float -mno-abicalls -fno-pic -G0`, `-ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-unwind-tables -fno-asynchronous-unwind-tables -nostdlib -fno-stack-protector`. Linker: `ld.lld -T placeholder.ld --defsym=S650_LOAD_ADDRESS=... --static --nmagic --no-dynamic-linker -z norelro --build-id=none`.
Things deliberately **not** done: no `-mcpu` naming a 4KE (unconfirmed), no MIPS16e, no DSP, no hard-float, no PIC, no startup code, no stack setup, no `.bss` clearing.

## Results: the first target executable
`target/tests/hello_s650.cpp`: freestanding, identifier string `S650-HELLO-0001|EXPERIMENTAL|NOT-CAMERA-VERIFIED`, entry `_start`, writes eight words into `s650_test_buffer` and spins. No stack, heap, calls, libc, exceptions or RTTI (checked in the disassembly: no `$sp`/`$fp` use, no calls, zero undefined symbols).

| Variant | Byte order | ELF flags | Entry | ELF bytes | Flat .bin | text/data/bss | SHA-256 (first 12) |
|---|---|---|---|---|---|---|---|
| `mips-mips1-placeholder` | big endian | 0x1001, noreorder, o32 | 0x80000000 | 1024 | 168 | 145/8/32 | f06268c7d66f |
| `mips-mips1-qemu` | big endian | 0x1001, noreorder, o32 | 0x80100000 | 1024 | 168 | 145/8/32 | 3d37bb05e8a9 |
| `mips-mips32-placeholder` | big endian | 0x50001001, noreorder, o32, mips32 | 0x80000000 | 1024 | 168 | 145/8/32 | 6c93e9cfde81 |
| `mips-mips32-qemu` | big endian | 0x50001001, noreorder, o32, mips32 | 0x80100000 | 1024 | 168 | 145/8/32 | dd80188c2d04 |
| `mips-mips32r2-placeholder` | big endian | 0x70001001, noreorder, o32, mips32r2 | 0x80000000 | 1024 | 168 | 145/8/32 | 70aca99718ab |
| `mips-mips32r2-qemu` | big endian | 0x70001001, noreorder, o32, mips32r2 | 0x80100000 | 1024 | 168 | 145/8/32 | 78e6eb43afa1 |
| `mipsel-mips1-placeholder` | little endian | 0x1001, noreorder, o32 | 0x80000000 | 1024 | 168 | 145/8/32 | 95c5b77a4186 |
| `mipsel-mips1-qemu` | little endian | 0x1001, noreorder, o32 | 0x80100000 | 1024 | 168 | 145/8/32 | ee84d659c906 |
| `mipsel-mips32-placeholder` | little endian | 0x50001001, noreorder, o32, mips32 | 0x80000000 | 1024 | 168 | 145/8/32 | 8566578644f9 |
| `mipsel-mips32-qemu` | little endian | 0x50001001, noreorder, o32, mips32 | 0x80100000 | 1024 | 168 | 145/8/32 | 0bcdf1bb8c62 |
| `mipsel-mips32r2-placeholder` | little endian | 0x70001001, noreorder, o32, mips32r2 | 0x80000000 | 1024 | 168 | 145/8/32 | fb28dca32b1a |
| `mipsel-mips32r2-qemu` | little endian | 0x70001001, noreorder, o32, mips32r2 | 0x80100000 | 1024 | 168 | 145/8/32 | e859e5d14ee5 |

Common properties: compiler Homebrew clang 23.1.4; ELF32, type EXEC, machine "MIPS R3000" (the ELF `EM_MIPS` code used for all 32-bit MIPS), ABI o32, `.text` 96 bytes, `.rodata` 49, one 32-byte `.s650_testbuf`; an 8-byte `.got` is emitted by the linker for the o32 ABI and is unused. No external dependencies; no runtime assumptions beyond: the program is loaded at its link address, execution starts at the entry address, and memory at the link address is writable.

## Verification
- **Static:** `inspect.sh` per variant (headers, sections, symbols, disassembly, no stack use, no calls, no undefined symbols).
- **Emulation (QEMU 11.0.2, `malta`, CPU `4KEc`):** all six `*-qemu` variants (both byte orders, all three ISA levels) executed, and the test buffer read back through the QEMU monitor equals `53363530 48454C4C 1 2 3 4 5 FFFFFFFF`. **PASS 6/6.**
- **Negative controls:** with the CPU paused (`-S`) the same read returns all zeros (so the check can fail); a little-endian image on the big-endian machine is rejected by QEMU ("The image has incorrect endianness").
- **Not verified:** execution on the ZR36473 or on any camera. **QEMU's 4KEc model is not the camera.**

## Application portability result (`port-matrix.sh`)
Compile-only attempt of every application source for the target with no standard library: **0 of 24 compile.** First blockers: `<cstdint>`/`<string>`/`<memory>` are not available (the compiler does provide C's `<stdint.h>`/`<stddef.h>`; the C++ wrappers and containers are absent). Adding Homebrew's libc++ headers also fails (it is configured for Apple platforms). Conclusion: reusing the application needs either a target-built C++ standard library plus C runtime, or a refactor to fixed buffers; `Types.h` itself pulls in `<string>`. This is a software-only task that does not depend on camera evidence and is the right next *code* step once an execution path exists.

## What is missing for a camera-compatible executable
1. The core's real ISA/endianness/FPU (then only one variant remains).
2. The load mechanism and format (flat image? container? signed? compressed?).
3. A valid load/entry address and memory that is safe to use.
4. Whatever startup state the loader leaves (stack, caches, interrupts) - or evidence that none is needed.
5. A way to observe the result (display, LED, memory read) - see `DISPLAY-INTEGRATION.md`.
