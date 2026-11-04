#!/usr/bin/env python3
"""EMULATION-ONLY checks of the experimental MIPS binaries.

1. mips_sample (all 6 *-qemu variants) on QEMU malta/4KEc: results are compared with values computed independently on the host
   (validates compiler output for every endianness/ISA hypothesis).
2. isa_probe (both byte orders) on every MIPS32-class CPU model QEMU offers: shows which instruction classes each model runs.

QEMU's CPU models are generic MIPS cores. None is the Zoran ZR36473. Agreement or disagreement here is NOT evidence about the camera.
"""
import glob, os, re, struct, subprocess, sys, time, zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LLVM = os.environ.get("LLVM_PREFIX", "/opt/homebrew/opt/llvm")
M = 0xFFFFFFFF

def endian_of(elf):
    with open(elf, "rb") as f: h = f.read(6)
    return "little" if h[5] == 1 else "big"

def sym(elf, name):
    for line in subprocess.run([f"{LLVM}/bin/llvm-nm", elf], capture_output=True, text=True, check=True).stdout.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] == name: return int(p[0], 16)
    raise KeyError(name)

def qemu_run(elf, cpu, wait=1.6):
    qemu = "qemu-system-mipsel" if endian_of(elf) == "little" else "qemu-system-mips"
    phys = sym(elf, "s650_test_buffer") & 0x1FFFFFFF
    cmd = [qemu, "-M", "malta", "-cpu", cpu, "-m", "64", "-display", "none", "-serial", "none", "-monitor", "stdio", "-kernel", elf]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    time.sleep(wait)
    try: out, _ = p.communicate(f"xp /16wx 0x{phys:x}\nquit\n", timeout=15)
    except subprocess.TimeoutExpired: p.kill(); return None
    words = []
    for line in out.splitlines():
        m = re.match(r"\s*[0-9a-fA-F]+:\s+((?:0x[0-9a-fA-F]+\s*)+)", line)
        if m: words += [int(x, 16) for x in m.group(1).split()]
    return words[:16] if len(words) >= 16 else None

# ---------- host-side reference for mips_sample ----------
def lcg(s): return (s * 1664525 + 1013904223) & M
def sample_expected(endian):
    seed = 12345; data = []
    for _ in range(64): seed = lcg(seed); data.append(seed >> 8)
    data.sort(); sorted_sum = sum((i + 1) * v for i, v in enumerate(data)) & M
    seed = 777; bs = bytearray()
    for _ in range(256): seed = lcg(seed); bs.append(seed >> 24)
    crc = zlib.crc32(bytes(bs)) & M
    def jt(x):
        c = x & 7
        return [(x + 11) & M, (x * 3) & M, x ^ 0x5A5A, (x - 7) & M, (x << 2) & M, x >> 1, (~x) & M, x | 0x100][c]
    jts = sum(jt((i * 2654435761) & M) for i in range(32)) & M
    fp = 5
    for i in range(9): fp = [(fp + 1) & M, (fp * 2) & M, (-fp) & M][i % 3]
    un = struct.unpack("<I" if endian == "little" else ">I", bytes([0x78, 0x56, 0x34, 0x12]) if endian == "little" else bytes([0x12, 0x34, 0x56, 0x78]))[0]
    # the bytes in memory are 78 56 34 12; a correct unaligned load yields 0x12345678 (LE) or 0x78563412 (BE)
    un = 0x12345678 if endian == "little" else 0x78563412
    a, b = 0xDEADBEEF, 0x12345678
    d = b | 1
    return [sorted_sum, crc, 6765, jts, fp, un, (a * b) >> 32 & M, ((a // d) ^ (a % d)) & M]

# ---------- host-side reference for isa_probe ----------
def probe_expected(endian):
    x, y = 0xA5C3F00F, 7
    rotr = ((x >> 12) | (x << 20)) & M
    wsbh = ((x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8) & M
    unal = 0x44332211 if endian == "little" else 0x11223344
    return {1: ((x << 4) + y) & M, 2: (x * y) & M, 3: 29, 4: 7, 5: (x >> 4) & 0xFF, 6: 0xFFFF07FF, 7: 0x0F, 8: wsbh, 9: rotr, 10: unal}

PROBE_NAMES = {1: "MIPS-I sll/addu", 2: "MIPS32 mul", 3: "MIPS32 clz", 4: "MIPS32 movn", 5: "R2 ext", 6: "R2 ins", 7: "R2 seb", 8: "R2 wsbh", 9: "R2 rotr", 10: "MIPS-I lwl/lwr"}

def cpu_list(qemu):
    out = subprocess.run([qemu, "-cpu", "help"], capture_output=True, text=True).stdout
    return [l.strip() for l in out.splitlines() if l.strip() and not l.lower().startswith("available")]

def main():
    fails = 0
    print("== 1. mips_sample on QEMU malta vs host-computed reference (R2 model 4KEc, R1 models 4KEcR1 / 4KEmR1 / 4Kc) ==")
    for cpu in ("4KEc", "4KEcR1", "4KEmR1", "4Kc"):
        for elf in sorted(glob.glob(f"{ROOT}/build-target/*-qemu/mips_sample.elf")):
            name = os.path.basename(os.path.dirname(elf)); e = endian_of(elf)
            w = qemu_run(elf, cpu)
            exp = sample_expected(e)
            ok = w is not None and w[:8] == exp and w[15] == 0x600DF00D
            fails += not ok
            print(("PASS " if ok else "FAIL ") + f"{name:22s} cpu={cpu}", "" if ok else f"got={[hex(x) for x in (w or [])[:8]]} want={[hex(x) for x in exp]}")
    print("\n== 2. isa_probe on every QEMU MIPS CPU model (EMULATION ONLY) ==")
    probes = {}
    for e, d in (("little", "mipsel-mips32-qemu"), ("big", "mips-mips32-qemu")):
        probes[e] = f"{ROOT}/build-target/{d}/isa_probe.elf"
    for e, qemu in (("little", "qemu-system-mipsel"), ("big", "qemu-system-mips")):
        exp = probe_expected(e)
        print(f"\n-- {e}-endian ({qemu}); probes: " + ", ".join(f"{k}={v}" for k, v in PROBE_NAMES.items()))
        print(f"{'CPU model':18s} {'progress':8s} first-failing probe / notes")
        for cpu in cpu_list(qemu):
            w = qemu_run(probes[e], cpu, wait=1.2)
            if w is None: print(f"{cpu:18s} {'n/a':8s} (could not run)"); continue
            done = w[0]; complete = w[15] == 0x600DF00D
            bad = [k for k in range(1, 11) if k <= done and w[k] != exp[k]]
            note = "all 10 probes executed" if complete and not bad else ""
            if not complete: note = f"stopped after probe {done}; next probe = {PROBE_NAMES.get(done + 1, '?')}"
            if bad: note += f"  WRONG RESULT in {[PROBE_NAMES[k] for k in bad]}"
            print(f"{cpu:18s} {done:<8d} {note}")
    print("\nEMULATION ONLY: QEMU CPU models are generic MIPS cores, not the ZR36473.")
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())
