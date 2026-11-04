#!/usr/bin/env python3
"""Compares the experimental MIPS binaries across the endianness / ISA hypotheses and writes docs/generated/variant-comparison.md.
All findings describe OUR compiler output. They say nothing about the ZR36473, whose core and ISA are unknown."""
import os, re, struct, subprocess, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "firmware-analysis"))
from fwa import elfinfo, mips

BT = os.path.join(ROOT, "build-target")
LLVM = os.environ.get("LLVM_PREFIX", "/opt/homebrew/opt/llvm")
PROGS = ["hello_s650", "mips_sample", "isa_probe"]
ENDS = [("mipsel", "little"), ("mips", "big")]
ISAS = ["mips1", "mips32", "mips32r2"]

def read(p):
    with open(p, "rb") as f: return f.read()

def text_bytes(elf):
    e = elfinfo.parse(elf)
    return b"".join(elf[s["offset"]:s["offset"] + s["size"]] for s in e["sections"] if s["flags"] & 4 and s["type"] == 1), e

def mnemonics(path):
    out = subprocess.run([f"{LLVM}/bin/llvm-objdump", "-d", "--no-show-raw-insn", path], capture_output=True, text=True).stdout
    m = []
    for line in out.splitlines():
        mm = re.match(r"\s*[0-9a-f]+:\s+(\S+)", line)
        if mm and mm.group(1) != "<unknown>": m.append(mm.group(1))
    return m

def swap_words(b):
    return b"".join(b[i:i + 4][::-1] for i in range(0, len(b) - len(b) % 4, 4))

def main():
    L = ["# Generated: MIPS variant comparison", "",
         "Produced by `tools/target-build/compare-variants.py` from the binaries in `build-target/`. **Describes our compiler's output only; says nothing about the ZR36473.**", ""]
    data = {}
    for prog in PROGS:
        for t, e in ENDS:
            for isa in ISAS:
                p = f"{BT}/{t}-{isa}-qemu/{prog}.elf"
                if not os.path.exists(p): continue
                elf = read(p); tx, info = text_bytes(elf)
                mn = mnemonics(p)
                data[(prog, e, isa)] = {"text": tx, "mn": mn, "flags": hex(info["flags"]), "arch": info["mips_arch_flag"], "path": p}
    for prog in PROGS:
        L += [f"## {prog}", "", "| endian | -march | ELF arch flag | text bytes | instructions | distinct mnemonics | beyond MIPS-I (lower bound) | lwl/lwr/swl/swr | calls (jal/jalr) |", "|---|---|---|---|---|---|---|---|---|"]
        for e in ("little", "big"):
            for isa in ISAS:
                d = data.get((prog, e, isa))
                if not d: continue
                mnset = set(d["mn"])
                bound = mips.isa_lower_bound(mnset)
                beyond = sorted(mnset & (mips.MIPS2_PLUS | mips.MIPS32_R1 | mips.MIPS32_R2))
                un = sum(d["mn"].count(x) for x in mips.UNALIGNED)
                calls = sum(d["mn"].count(x) for x in ("jal", "jalr", "jalr.hb"))
                L.append(f"| {e} | {isa} | {d['arch']} | {len(d['text'])} | {len(d['mn'])} | {len(mnset)} | {bound['lower_bound'].split(' ')[0]} {beyond if beyond else ''} | {un} | {calls} |")
        L.append("")
        # endianness relation
        L += ["**Byte order relation** (same ISA): is the big-endian `.text` the word-swapped little-endian `.text`?", ""]
        for isa in ISAS:
            a, b = data.get((prog, "little", isa)), data.get((prog, "big", isa))
            if not (a and b): continue
            sw = swap_words(a["text"]); same = sw == b["text"]
            diff = sum(1 for i in range(0, min(len(sw), len(b["text"])), 4) if sw[i:i + 4] != b["text"][i:i + 4])
            L.append(f"- {isa}: {'identical after swapping each 32-bit word' if same else f'{diff} of {len(sw)//4} words differ after swapping (data/immediates or different codegen)'}; same instruction sequence: {a['mn'] == b['mn']}")
        L += ["", "**ISA relation** (same byte order): how does the code change between `-march` levels?", ""]
        for e in ("little", "big"):
            base = data.get((prog, e, "mips1"))
            for isa in ("mips32", "mips32r2"):
                d = data.get((prog, e, isa))
                if not (base and d): continue
                only_new = sorted(set(d["mn"]) - set(base["mn"])); only_old = sorted(set(base["mn"]) - set(d["mn"]))
                L.append(f"- {e}: mips1 -> {isa}: text {len(base['text'])} -> {len(d['text'])} bytes; instructions {len(base['mn'])} -> {len(d['mn'])}; mnemonics only in {isa}: {only_new or 'none'}; only in mips1: {only_old or 'none'}")
        L.append("")
    # placeholder vs qemu link address
    L += ["## Link address effect", "", "`placeholder` (0x80000000) and `qemu` (0x80100000) builds differ only in absolute address immediates (`lui`/`ori`/`j`/`jal`). Words differing between them (mips_sample, little, mips32):", ""]
    a = read(f"{BT}/mipsel-mips32-placeholder/mips_sample.elf"); b = read(f"{BT}/mipsel-mips32-qemu/mips_sample.elf")
    ta, _ = text_bytes(a); tb, _ = text_bytes(b)
    diff = [i // 4 for i in range(0, min(len(ta), len(tb)), 4) if ta[i:i + 4] != tb[i:i + 4]]
    L.append(f"{len(diff)} of {len(ta)//4} words (all other instructions identical).")
    L.append("")
    out = os.path.join(ROOT, "docs", "generated"); os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "variant-comparison.md"), "w") as f: f.write("\n".join(L))
    print("\n".join(L))

if __name__ == "__main__":
    main()
