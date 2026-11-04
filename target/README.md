# Experimental S650 target

> **Nothing here has run on a Sony DSC-S650.** The processor core, ISA revision, endianness, ABI, memory map, load mechanism and executable format of the camera are unknown. The code here is a generic bare-metal MIPS program built under labelled hypotheses, verified only by static inspection and by an emulator that is not the camera.

- `tests/hello_s650.cpp` - smallest test program (no stack, heap, libc, exceptions, RTTI; fills an 8-word buffer and spins).
- `linker/placeholder.ld` - **placeholder** linker script; the link address is arbitrary (`0x80000000`, or `0x80100000` for the emulator build).
- `../cmake/s650-target.cmake` - experimental toolchain file.
- `../tools/target-build/` - `build.sh` (12 variants), `inspect.sh`, `qemu-verify.py`, `port-matrix.sh`.
- Documentation: `../docs/TARGET-ARCHITECTURE.md`, `CROSS-COMPILATION.md`, `EXECUTION-ENVIRONMENT.md`, `FIRMWARE-ANALYSIS.md`, `DISPLAY-INTEGRATION.md`, `RECOVERY-REQUIREMENTS.md`.

No startup assembly exists because nothing is known about the state the camera would hand over (stack, caches, interrupts). It will be added only when evidence supports it. The desktop build in `../CMakeLists.txt` is independent of this directory.
