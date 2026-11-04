"""Minimal pure-Python ELF reader (headers, sections, program headers). Enough for identification, not a linker."""
import struct

MACHINES = {0: "none", 3: "x86", 8: "MIPS", 40: "ARM", 62: "x86-64", 183: "AArch64", 243: "RISC-V"}
MIPS_ARCH = {0x00000000: "mips1", 0x10000000: "mips2", 0x20000000: "mips3", 0x30000000: "mips4", 0x40000000: "mips5",
             0x50000000: "mips32", 0x60000000: "mips64", 0x70000000: "mips32r2", 0x80000000: "mips64r2", 0x90000000: "mips32r6", 0xA0000000: "mips64r6"}

def parse(data: bytes):
    """Returns a dict, or None if not a structurally valid ELF."""
    if len(data) < 52 or data[:4] != b"\x7fELF": return None
    cls, enc, ver = data[4], data[5], data[6]
    if cls not in (1, 2) or enc not in (1, 2) or ver != 1: return None
    e = "<" if enc == 1 else ">"
    if cls == 1:
        (typ, mach, _v, entry, phoff, shoff, flags, ehsize, phentsize, phnum, shentsize, shnum, shstrndx) = struct.unpack(e + "HHIIIIIHHHHHH", data[16:52])
    else:
        if len(data) < 64: return None
        (typ, mach, _v, entry, phoff, shoff, flags, ehsize, phentsize, phnum, shentsize, shnum, shstrndx) = struct.unpack(e + "HHIQQQIHHHHHH", data[16:64])
    if ehsize not in (52, 64): return None
    info = {"class": 32 if cls == 1 else 64, "endian": "little" if enc == 1 else "big", "type": {1: "REL", 2: "EXEC", 3: "DYN", 4: "CORE"}.get(typ, str(typ)),
            "machine": MACHINES.get(mach, str(mach)), "machine_code": mach, "entry": entry, "flags": flags, "phnum": phnum, "shnum": shnum}
    if mach == 8:
        info["mips_arch_flag"] = MIPS_ARCH.get(flags & 0xF0000000, "unknown")
        info["mips_abi"] = {0: "none", 0x1000: "o32", 0x2000: "o64"}.get(flags & 0x3000, "n32/n64/other")
    # section table (validated against file size)
    secs = []
    if shoff and shnum and shentsize in (40, 64) and shoff + shnum * shentsize <= len(data):
        raw = []
        for i in range(shnum):
            o = shoff + i * shentsize
            if cls == 1: f = struct.unpack(e + "IIIIIIIIII", data[o:o + 40]); name, typ_, fl, addr, off, size = f[0], f[1], f[2], f[3], f[4], f[5]
            else: f = struct.unpack(e + "IIQQQQIIQQ", data[o:o + 64]); name, typ_, fl, addr, off, size = f[0], f[1], f[2], f[3], f[4], f[5]
            raw.append((name, typ_, fl, addr, off, size))
        names = b""
        if 0 <= shstrndx < len(raw):
            _, _, _, _, off, size = raw[shstrndx]; names = data[off:off + size]
        for (name, typ_, fl, addr, off, size) in raw:
            nm = names[name:names.find(b"\0", name)].decode("ascii", "replace") if names and name < len(names) else ""
            secs.append({"name": nm, "type": typ_, "flags": fl, "addr": addr, "offset": off, "size": size})
    info["sections"] = secs
    return info
