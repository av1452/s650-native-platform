#!/usr/bin/env python3
"""Runs the *-qemu hello binaries on an EMULATED generic MIPS32 core (QEMU 'malta', CPU 4KEc) and reads the test buffer back.

This validates the toolchain output (ISA, endianness, entry, linker script) against an independent implementation of
MIPS. It is NOT evidence about the Zoran ZR36473 or the camera: QEMU's malta board and 4KEc model are not the camera.
"""
import glob, os, re, subprocess, sys, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EXPECTED = [0x53363530, 0x48454C4C, 1, 2, 3, 4, 5, 0xFFFFFFFF]

def elf_endian(path):
    with open(path, "rb") as f:
        h = f.read(6)
    assert h[:4] == b"\x7fELF" and h[4] == 1, "not a 32-bit ELF"
    return "little" if h[5] == 1 else "big"

def symbol_addr(llvm, elf, name):
    out = subprocess.run([f"{llvm}/bin/llvm-nm", elf], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] == name:
            return int(p[0], 16)
    raise KeyError(name)

def run(elf, cpu, llvm):
    endian = elf_endian(elf)
    qemu = "qemu-system-mipsel" if endian == "little" else "qemu-system-mips"
    vaddr = symbol_addr(llvm, elf, "s650_test_buffer")
    phys = vaddr & 0x1FFFFFFF
    cmd = [qemu, "-M", "malta", "-cpu", cpu, "-m", "64", "-display", "none", "-serial", "none", "-monitor", "stdio", "-kernel", elf]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    time.sleep(2.0)                                   # let the guest run
    try:
        out, _ = p.communicate(f"xp /8wx 0x{phys:x}\nquit\n", timeout=15)
    except subprocess.TimeoutExpired:
        p.kill(); return None, "qemu timeout"
    words = []
    for line in out.splitlines():
        m = re.match(r"\s*[0-9a-fA-F]+:\s+((?:0x[0-9a-fA-F]+\s*)+)", line)
        if m:
            words += [int(x, 16) for x in m.group(1).split()]
    return words[:8], out

def main():
    llvm = os.environ.get("LLVM_PREFIX", "/opt/homebrew/opt/llvm")
    elfs = sorted(glob.glob(os.path.join(ROOT, "build-target", "*-qemu", "hello_s650.elf")))
    if not elfs:
        print("no *-qemu binaries; run tools/target-build/build.sh first"); return 2
    failed = 0
    for elf in elfs:
        name = os.path.basename(os.path.dirname(elf))
        for cpu in ("4KEc",):
            words, raw = run(elf, cpu, llvm)
            ok = words == EXPECTED
            failed += not ok
            print(f"{'PASS' if ok else 'FAIL'}  {name:24s} cpu={cpu}  buffer={['0x%08X' % w for w in words] if words else raw}")
    print("\nEMULATION ONLY (QEMU malta / 4KEc). Says nothing about the real camera.")
    return 1 if failed else 0

if __name__ == "__main__":
    sys.exit(main())
