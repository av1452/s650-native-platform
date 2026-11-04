"""Signature scanning with validation. A hit is 'recognized' only if structure beyond the magic bytes checks out."""
import bz2, struct, zlib, lzma
from . import elfinfo

M32 = 0xFFFFFFFF

def _hit(name, level, **detail):
    return {"name": name, "level": level, "detail": detail}

# ---------------- validators: (data, offset) -> hit dict or None ----------------
def v_elf(d, o):
    info = elfinfo.parse(d[o:o + 4096 * 64]) if d[o:o + 4] == b"\x7fELF" else None
    if not info: return None
    keep = {k: info[k] for k in ("class", "endian", "type", "machine", "entry", "mips_arch_flag", "mips_abi") if k in info}
    keep["entry"] = hex(keep["entry"])
    return _hit("ELF", "recognized", **keep)

def v_gzip(d, o):
    if d[o + 2:o + 3] != b"\x08" or len(d) < o + 18 or d[o + 3] & 0xE0: return None
    flg, pos = d[o + 3], o + 10
    if flg & 4:
        if pos + 2 > len(d): return None
        pos += 2 + struct.unpack("<H", d[pos:pos + 2])[0]
    for bit in (8, 16):
        if flg & bit:
            z = d.find(b"\0", pos)
            if z < 0: return None
            pos = z + 1
    if flg & 2: pos += 2
    try:
        dec = zlib.decompressobj(-15); out = dec.decompress(d[pos:pos + (1 << 20)], 1 << 16)
        return _hit("gzip", "recognized", decompressed_prefix_bytes=len(out), complete=dec.eof)
    except zlib.error:
        return _hit("gzip", "possible", reason="header ok, deflate stream did not decode")

def v_zlib(d, o):
    cmf, flg = d[o], d[o + 1] if o + 1 < len(d) else 0
    if cmf & 0x0F != 8 or (cmf * 256 + flg) % 31 or cmf >> 4 > 7 or flg & 0x20: return None
    try:
        dec = zlib.decompressobj(); out = dec.decompress(d[o:o + (1 << 20)], 1 << 16)
        if len(out) >= 16 or dec.eof: return _hit("zlib stream", "recognized", decompressed_prefix_bytes=len(out), complete=dec.eof)
    except zlib.error:
        pass
    return None            # two-byte magics are too weak to report without a successful decode

def v_bzip2(d, o):
    if not (d[o + 3:o + 4] in b"123456789" and d[o + 4:o + 10] == b"\x31\x41\x59\x26\x53\x59"): return None
    try:
        out = bz2.BZ2Decompressor().decompress(d[o:o + (1 << 18)]); return _hit("bzip2", "recognized", decompressed_prefix_bytes=len(out))
    except (OSError, ValueError):
        return _hit("bzip2", "possible", reason="block magic present, decode failed (truncated?)")

def v_xz(d, o):
    if len(d) < o + 12: return None
    flags = d[o + 6:o + 8]; crc = struct.unpack("<I", d[o + 8:o + 12])[0]
    if flags[0] != 0 or (zlib.crc32(flags) & M32) != crc: return None
    return _hit("xz", "recognized", check_type=flags[1] & 0x0F)

def v_png(d, o):
    if d[o + 12:o + 16] != b"IHDR" or struct.unpack(">I", d[o + 8:o + 12])[0] != 13: return None
    crc = struct.unpack(">I", d[o + 29:o + 33])[0] if len(d) >= o + 33 else None
    if crc is None or (zlib.crc32(d[o + 12:o + 29]) & M32) != crc: return _hit("PNG", "possible", reason="IHDR CRC mismatch")
    w, h = struct.unpack(">II", d[o + 16:o + 24]); return _hit("PNG", "recognized", width=w, height=h)

def v_jpeg(d, o):
    if len(d) < o + 4 or d[o + 3] < 0xC0: return None
    end = d.find(b"\xff\xd9", o + 4, o + (1 << 24))
    return _hit("JPEG", "recognized" if end > 0 else "possible", marker=hex(d[o + 3]), end_of_image_found=end > 0, length=(end + 2 - o) if end > 0 else None)

def v_bmp(d, o):
    if len(d) < o + 26: return None
    size, off = struct.unpack("<II", d[o + 2:o + 10])[0], struct.unpack("<I", d[o + 10:o + 14])[0]
    hdr = struct.unpack("<I", d[o + 14:o + 18])[0]
    if hdr not in (12, 40, 52, 56, 108, 124) or off < 14 + hdr or off > max(size, 1 << 28): return None
    return _hit("BMP", "recognized" if size and o + size <= len(d) else "possible", header_size=hdr, declared_size=size)

def v_riff(d, o):
    kind = d[o + 8:o + 12]
    if kind not in (b"AVI ", b"WAVE", b"WEBP"): return None
    size = struct.unpack("<I", d[o + 4:o + 8])[0]
    return _hit("RIFF/" + kind.decode().strip(), "recognized" if o + 8 + size <= len(d) else "possible", declared_size=size)

def v_zip(d, o):
    if len(d) < o + 30: return None
    ver, flg, meth, _t, _dt, _crc, csz, usz, nlen, elen = struct.unpack("<HHHHHIIIHH", d[o + 4:o + 30])
    if ver > 63 or meth not in (0, 8, 9, 12, 14, 93, 95, 98) or nlen == 0 or o + 30 + nlen > len(d): return None
    return _hit("ZIP", "recognized", first_entry=d[o + 30:o + 30 + nlen].decode("utf-8", "replace"), method=meth)

def v_7z(d, o):
    if len(d) < o + 32: return None
    want = struct.unpack("<I", d[o + 8:o + 12])[0]
    return _hit("7-Zip", "recognized" if (zlib.crc32(d[o + 12:o + 32]) & M32) == want else "possible")

def v_tar(d, o):
    base = o - 257
    if base < 0 or len(d) < base + 512: return None
    hdr = bytearray(d[base:base + 512]); stored = hdr[148:156]
    hdr[148:156] = b" " * 8
    try: want = int(bytes(stored).split(b"\0")[0].strip() or b"0", 8)
    except ValueError: return None
    return _hit("tar (ustar)", "recognized" if sum(hdr) == want else "possible", first_name=d[base:base + 100].split(b"\0")[0].decode("latin1"), _base=base)

def v_uimage(d, o):
    if len(d) < o + 64: return None
    hdr = bytearray(d[o:o + 64]); hcrc = struct.unpack(">I", bytes(hdr[4:8]))[0]; hdr[4:8] = b"\0\0\0\0"
    name = bytes(hdr[32:64]).split(b"\0")[0].decode("latin1")
    ok = (zlib.crc32(bytes(hdr)) & M32) == hcrc
    arch = {2: "ARM", 5: "MIPS", 3: "i386", 7: "PowerPC", 8: "MIPS64"}.get(hdr[29], hdr[29])
    return _hit("U-Boot uImage header", "recognized" if ok else "possible", header_crc_valid=ok, image_name=name, arch=arch,
                data_size=struct.unpack(">I", bytes(hdr[12:16]))[0], load=hex(struct.unpack(">I", bytes(hdr[16:20]))[0]), entry=hex(struct.unpack(">I", bytes(hdr[20:24]))[0]))

def v_fdt(d, o):
    if len(d) < o + 40: return None
    total, off_struct, off_str, _r, ver, last = struct.unpack(">IIIIII", d[o + 4:o + 28])
    if not (17 <= ver <= 17 or 1 <= last <= ver) or total < 40 or total > (1 << 26) or off_struct >= total or off_str >= total: return None
    return _hit("Flattened Device Tree", "recognized" if o + total <= len(d) else "possible", totalsize=total, version=ver)

def v_squashfs(d, o):
    if len(d) < o + 96: return None
    le = d[o:o + 4] == b"hsqs"; e = "<" if le else ">"
    blk, major = struct.unpack(e + "I", d[o + 12:o + 16])[0], struct.unpack(e + "H", d[o + 28:o + 30])[0]
    ok = major == 4 and blk >= 4096 and blk & (blk - 1) == 0 and blk <= 1 << 20
    return _hit("SquashFS", "recognized" if ok else "possible", endian="little" if le else "big", block_size=blk, version_major=major)

def v_cramfs(d, o):
    if len(d) < o + 48: return None
    return _hit("cramfs", "recognized" if d[o + 16:o + 32] == b"Compressed ROMFS" else "possible", endian="little" if d[o:o + 4] == b"\x45\x3d\xcd\x28" else "big")

def v_romfs(d, o):
    return _hit("romfs", "recognized", volume=d[o + 16:o + 32].split(b"\0")[0].decode("latin1")) if len(d) >= o + 32 else None

def v_ubi(d, o):
    return _hit("UBI erase-counter header", "possible", reason="header CRC not verified") if d[o:o + 4] == b"UBI#" and d[o + 4:o + 5] == b"\x01" else None

def v_jffs2(d, o):
    return None   # a 2-byte magic is far too weak on its own; see scan_fs_extra for aggregated detection

def v_pe(d, o):
    if len(d) < o + 64: return None
    pe = struct.unpack("<I", d[o + 60:o + 64])[0]
    return _hit("PE/MZ executable", "recognized") if 0 < pe < 4096 and d[o + pe:o + pe + 4] == b"PE\0\0" else None

def v_macho(d, o):
    return _hit("Mach-O", "possible", reason="magic only")

# (magic bytes, validator, offset of magic within the structure)
SIGS = [
    (b"\x7fELF", v_elf), (b"\x1f\x8b", v_gzip), (b"BZh", v_bzip2), (b"\xfd7zXZ\x00", v_xz),
    (b"\x89PNG\r\n\x1a\n", v_png), (b"\xff\xd8\xff", v_jpeg), (b"BM", v_bmp), (b"RIFF", v_riff),
    (b"PK\x03\x04", v_zip), (b"7z\xbc\xaf\x27\x1c", v_7z), (b"ustar", v_tar),
    (b"\x27\x05\x19\x56", v_uimage), (b"\xd0\x0d\xfe\xed", v_fdt), (b"hsqs", v_squashfs), (b"sqsh", v_squashfs),
    (b"\x45\x3d\xcd\x28", v_cramfs), (b"\x28\xcd\x3d\x45", v_cramfs), (b"-rom1fs-", v_romfs), (b"UBI#", v_ubi),
    (b"MZ", v_pe), (b"\xfe\xed\xfa\xce", v_macho), (b"\xce\xfa\xed\xfe", v_macho), (b"\xcf\xfa\xed\xfe", v_macho),
]
ZLIB_FIRST = (0x78,)

def _fat(d, o):
    """FAT12/16/32 boot sector at o (sector-aligned)."""
    b = d[o:o + 512]
    if len(b) < 512 or b[510:512] != b"\x55\xaa" or b[0] not in (0xEB, 0xE9): return None
    bps, spc, rsv, nfats = struct.unpack("<HBHB", b[11:17])
    if bps not in (512, 1024, 2048, 4096) or spc == 0 or spc & (spc - 1) or rsv < 1 or nfats not in (1, 2): return None
    sectors = struct.unpack("<H", b[19:21])[0] or struct.unpack("<I", b[32:36])[0]
    if sectors == 0: return None
    fatsz = struct.unpack("<H", b[22:24])[0] or struct.unpack("<I", b[36:40])[0]
    kind = "FAT32" if struct.unpack("<H", b[22:24])[0] == 0 else "FAT12/16"
    return _hit("FAT file system boot sector (" + kind + ")", "recognized", bytes_per_sector=bps, sectors_per_cluster=spc, total_sectors=sectors,
                volume_bytes=sectors * bps, fat_sectors=fatsz, oem=b[3:11].decode("latin1").strip())

def _ext(d, o):
    s = d[o + 1024:o + 1024 + 264]
    if len(s) < 264 or s[56:58] != b"\x53\xef": return None
    log_bs = struct.unpack("<I", s[24:28])[0]
    return _hit("ext2/3/4 superblock", "recognized" if log_bs <= 6 else "possible", block_size=1024 << log_bs if log_bs <= 6 else None)

def _iso(d, o):
    return _hit("ISO 9660 volume descriptor", "recognized") if d[o + 0x8001:o + 0x8006] == b"CD001" else None

def scan(data: bytes, max_hits_per_sig: int = 64):
    hits = []
    for magic, fn in SIGS:
        pos, n = 0, 0
        while n < max_hits_per_sig:
            i = data.find(magic, pos)
            if i < 0: break
            pos = i + 1
            off = i - 257 if magic == b"ustar" else i
            if off < 0: continue
            h = fn(data, i)
            if h:
                if magic == b"ustar": off = h["detail"].pop("_base", off)
                hits.append({"offset": off, **h}); n += 1
    # zlib has only a 2-byte header: test candidates that start with 0x78 and a valid FLG
    pos, n = 0, 0
    while n < 32:
        i = data.find(b"\x78", pos)
        if i < 0 or i + 2 > len(data): break
        pos = i + 1
        if data[i + 1] in (0x01, 0x5E, 0x9C, 0xDA):
            h = v_zlib(data, i)
            if h: hits.append({"offset": i, **h}); n += 1
    # file systems that live at fixed positions relative to sector/partition starts
    for o in range(0, min(len(data), 1 << 28), 512):
        h = _fat(data, o)
        if h: hits.append({"offset": o, **h})
    for o in (0,):
        for fn in (_ext, _iso):
            h = fn(data, o)
            if h: hits.append({"offset": o, **h})
    hits.sort(key=lambda h: (h["offset"], h["name"]))
    return hits

def summarize(hits):
    rec = [h for h in hits if h["level"] == "recognized"]
    pos = [h for h in hits if h["level"] == "possible"]
    return {"recognized": len(rec), "possible": len(pos)}
