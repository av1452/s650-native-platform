import bz2, gzip, io, lzma, os, random, struct, subprocess, sys, tarfile, tempfile, unittest, zipfile, zlib
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from fwa import compare, elfinfo, entropy, hashing, mips, report, signatures, strings_
import s650fw

ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
BT = os.path.join(ROOT, "build-target")
M = 0xFFFFFFFF

def rnd(n, seed=1):
    r = random.Random(seed); return bytes(r.getrandbits(8) for _ in range(n))

def names(hits, level="recognized"):
    return {h["name"] for h in hits if h["level"] == level}

def png_bytes():
    ihdr = struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)
    def chunk(t, d): return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & M)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(b"\x00\xff\x00\x00")) + chunk(b"IEND", b"")

def fat_sector():
    b = bytearray(512)
    b[0:3] = b"\xEB\x3C\x90"; b[3:11] = b"MSDOS5.0"
    struct.pack_into("<HBHBH", b, 11, 512, 4, 1, 2, 512)
    struct.pack_into("<H", b, 19, 0); struct.pack_into("<I", b, 32, 49152); struct.pack_into("<H", b, 22, 48)
    b[510:512] = b"\x55\xAA"; return bytes(b)

def uimage(valid=True):
    h = bytearray(64); struct.pack_into(">IIIIIII", h, 0, 0x27051956, 0, 1177754400, 1000, 0x80008000, 0x80008000, 0)
    h[28] = 5; h[29] = 5; h[30] = 2; h[31] = 0; h[32:32 + 7] = b"testimg"
    crc = zlib.crc32(bytes(h)) & M
    struct.pack_into(">I", h, 4, crc if valid else crc ^ 1)
    return bytes(h)

class Hashing(unittest.TestCase):
    def test_known_vectors(self):
        h = hashing.hashes(b"abc")
        self.assertEqual(h["sha256"], "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        self.assertEqual(h["md5"], "900150983cd24fb0d6963f7d28e17f72"); self.assertEqual(h["crc32"], "352441c2")

class Entropy(unittest.TestCase):
    def test_extremes(self):
        self.assertEqual(entropy.shannon(b"\0" * 4096), 0.0)
        self.assertGreater(entropy.shannon(rnd(65536)), 7.9)
        t = entropy.shannon(b"the quick brown fox jumps over the lazy dog " * 100)
        self.assertTrue(3.5 < t < 5.0)
    def test_regions(self):
        data = b"\0" * 8192 + rnd(8192) + b"\0" * 4096
        p = entropy.profile(data)
        self.assertEqual([r["class"] for r in p["regions"]][0], "constant")
        self.assertIn("high-entropy", p["regions"][1]["class"])

class Signatures(unittest.TestCase):
    def check(self, blob, name, at=0, pad=0x1000):
        data = b"\xff" * pad + blob + b"\xff" * 64
        hits = [h for h in signatures.scan(data) if h["name"].startswith(name) and h["level"] == "recognized"]
        self.assertTrue(hits, "%s not recognized" % name)
        self.assertEqual(hits[0]["offset"], pad + at)
    def test_compression(self):
        payload = b"hello firmware " * 200
        self.check(gzip.compress(payload), "gzip"); self.check(zlib.compress(payload), "zlib")
        self.check(bz2.compress(payload), "bzip2"); self.check(lzma.compress(payload, format=lzma.FORMAT_XZ), "xz")
    def test_images_and_media(self):
        self.check(png_bytes(), "PNG")
        jpeg = b"\xff\xd8\xff\xe0\x00\x10JFIF\0\x01\x01\0\0\x01\0\x01\0\0" + b"\x12" * 40 + b"\xff\xd9"; self.check(jpeg, "JPEG")
        bmp = b"BM" + struct.pack("<IHHI", 70, 0, 0, 54) + struct.pack("<IiiHHIIiiII", 40, 2, 2, 1, 24, 0, 16, 0, 0, 0, 0) + b"\0" * 16; self.check(bmp, "BMP")
        avi = b"RIFF" + struct.pack("<I", 20) + b"AVI " + b"\0" * 16; self.check(avi, "RIFF/AVI")
    def test_archives(self):
        z = io.BytesIO()
        with zipfile.ZipFile(z, "w") as f: f.writestr("a.txt", "data" * 50)
        self.check(z.getvalue(), "ZIP")
        t = io.BytesIO()
        with tarfile.open(fileobj=t, mode="w") as f:
            ti = tarfile.TarInfo("x.bin"); ti.size = 10; f.addfile(ti, io.BytesIO(b"0123456789"))
        self.check(t.getvalue(), "tar")
    def test_boot_and_fs(self):
        self.check(uimage(True), "U-Boot uImage")
        fdt = struct.pack(">IIIIIIIII", 0xD00DFEED, 200, 56, 160, 40, 17, 16, 0, 40) + b"\0" * 160; self.check(fdt, "Flattened Device Tree")
        sq = b"hsqs" + struct.pack("<IIIIIHHHHH", 10, 0, 131072, 5, 0, 17, 0, 0, 0, 4) + b"\0" * 64
        sq = bytearray(b"hsqs" + b"\0" * 92); struct.pack_into("<I", sq, 12, 131072); struct.pack_into("<H", sq, 28, 4); self.check(bytes(sq), "SquashFS")
        self.check(fat_sector() + b"\0" * 1024, "FAT file system", pad=0x1000)
        sb = bytearray(2048); struct.pack_into("<I", sb, 1024 + 24, 2); sb[1024 + 56:1024 + 58] = b"\x53\xef"
        hits = signatures.scan(bytes(sb)); self.assertIn("ext2/3/4 superblock", names(hits))
    def test_possible_not_recognized(self):
        bad_gz = b"\x1f\x8b\x08\x00" + b"\0" * 6 + b"\xff" * 30
        h = [x for x in signatures.scan(b"\0" * 64 + bad_gz) if x["name"] == "gzip"]
        self.assertTrue(h and all(x["level"] == "possible" for x in h))
        h = [x for x in signatures.scan(uimage(False)) if x["name"].startswith("U-Boot")]
        self.assertTrue(h and h[0]["level"] == "possible" and not h[0]["detail"]["header_crc_valid"])
    def test_false_positive_control(self):
        # 2 MB of random data and of text must not yield any 'recognized' structure
        for data in (rnd(2 << 20, 7), (b"The quick brown fox. " * 100000)[: 2 << 20]):
            self.assertEqual(signatures.summarize(signatures.scan(data))["recognized"], 0)
    def test_png_bad_crc_is_possible(self):
        b = bytearray(png_bytes()); b[29] ^= 0xFF
        self.assertEqual({h["level"] for h in signatures.scan(bytes(b)) if h["name"] == "PNG"}, {"possible"})

class Strings(unittest.TestCase):
    def test_extract_and_label(self):
        d = b"\0\0\0Sony DSC version 1.10\0\0" + "Z\0o\0r\0a\0n\0 \0C\0O\0A\0C\0H\0".encode("latin1") + b"\xff" * 4
        s = strings_.extract(d, 6)
        self.assertTrue(any(x["encoding"] == "ascii" and "Sony" in x["text"] for x in s))
        self.assertTrue(any(x["encoding"] == "utf-16le" and "Zoran" in x["text"] for x in s))
        tags = {t for x in strings_.label(s) for t in x["tags"]}
        self.assertIn("vendor", tags); self.assertIn("version", tags)

class Compare(unittest.TestCase):
    def test_identical_diff_and_shift(self):
        a = rnd(8192, 3)
        self.assertTrue(compare.compare(a, a)["identical"])
        b = bytearray(a); b[5000] ^= 1
        c = compare.compare(a, bytes(b))
        self.assertFalse(c["identical"]); self.assertEqual(c["common_prefix"], 5000); self.assertEqual(len(c["differing_regions"]), 1)
        odd = rnd(100, 9) + a            # a 100-byte header: content moved off the 512-byte grid
        self.assertEqual(compare.compare(a, odd)["aligned_block_overlap"], 0)    # KNOWN LIMITATION: only grid-aligned moves are detected
        sh = rnd(1024, 9) + a            # a 512-multiple shift keeps every block hash
        self.assertGreaterEqual(compare.compare(a, sh)["aligned_block_overlap"], 15)

@unittest.skipUnless(os.path.isdir(BT) and mips.have_mc(), "needs build-target/ (tools/target-build/build.sh) and llvm-mc")
class MipsOnOwnBinaries(unittest.TestCase):
    def read(self, rel):
        with open(os.path.join(BT, rel), "rb") as f: return f.read()
    def text_of(self, data):
        e = elfinfo.parse(data); return b"".join(data[s["offset"]:s["offset"] + s["size"]] for s in e["sections"] if s["flags"] & 4 and s["type"] == 1)
    def test_elf_headers_match_variant_names(self):
        for d in os.listdir(BT):
            p = os.path.join(BT, d, "hello_s650.elf")
            if not os.path.isfile(p): continue
            e = elfinfo.parse(self.read(os.path.join(d, "hello_s650.elf")))
            self.assertEqual(e["machine"], "MIPS"); self.assertEqual(e["endian"], "little" if d.startswith("mipsel") else "big")
            self.assertEqual(e["mips_abi"], "o32")
            want = d.split("-")[1]; self.assertEqual(e["mips_arch_flag"], want)
    def test_raw_code_endianness_detection_ground_truth(self):
        correct = 0; total = 0
        for d in ("mipsel-mips1-qemu", "mipsel-mips32-qemu", "mipsel-mips32r2-qemu", "mips-mips1-qemu", "mips-mips32-qemu", "mips-mips32r2-qemu"):
            code = self.text_of(self.read(os.path.join(d, "mips_sample.elf")))
            r = mips.analyze(code); total += 1
            truth = "little" if d.startswith("mipsel") else "big"
            self.assertTrue(r["verdict"]["looks_like_mips32_code"], d)
            self.assertEqual(r["verdict"]["endianness_estimate"], truth, d); correct += 1
        self.assertEqual(correct, total)
    def test_isa_lower_bound_tracks_compile_flags(self):
        r1 = mips.analyze(self.text_of(self.read("mipsel-mips1-qemu/mips_sample.elf")))["verdict"]["isa"]["lower_bound"]
        self.assertTrue(r1.startswith("mips1"), r1)
        r2 = mips.analyze(self.text_of(self.read("mipsel-mips32r2-qemu/isa_probe.elf")), known_code=True)
        self.assertEqual(r2["verdict"]["isa"]["lower_bound"], "mips32r2")      # the probe deliberately contains R2 instructions
    def test_compiled_code_never_exceeds_declared_isa(self):
        # guard: -march=mips1 output must contain only MIPS-I instructions, -march=mips32 no Release 2 instructions (isa_probe is excluded:
        # it embeds newer instructions on purpose through .set directives)
        order = {"mips1": 0, "mips32": 1, "mips32r2": 2}
        rank = lambda lb: 0 if lb.startswith("mips1") else 1 if lb.startswith("mips32 ") or lb == "mips32" else 2 if lb.startswith("mips32r2") else 1
        for d in os.listdir(BT):
            for prog in ("hello_s650", "mips_sample"):
                p = os.path.join(d, prog + ".elf")
                if not os.path.isfile(os.path.join(BT, p)): continue
                declared = d.split("-")[1]
                got = mips.analyze(self.text_of(self.read(p)), known_code=True)["verdict"]["isa"]["lower_bound"]
                self.assertLessEqual(rank(got), order[declared], "%s: %s exceeds -march=%s" % (p, got, declared))
    def test_negative_controls(self):
        self.assertFalse(mips.analyze(rnd(65536, 5))["verdict"]["looks_like_mips32_code"])
        self.assertFalse(mips.analyze((b"The quick brown fox jumps over the lazy dog. " * 3000)[:65536])["verdict"]["looks_like_mips32_code"])
        self.assertFalse(mips.analyze(b"\0" * 65536)["verdict"]["looks_like_mips32_code"])
    def test_foreign_architecture_control(self):
        exe = os.path.join(ROOT, "build", "s650_native")
        if not os.path.isfile(exe): self.skipTest("desktop executable not built")
        with open(exe, "rb") as f: data = f.read()
        r = mips.analyze(data[0x8000:0x8000 + 65536])               # arm64 machine code from the desktop build
        self.assertFalse(r["verdict"]["looks_like_mips32_code"], r["verdict"]["summary"])
    def test_report_wording_never_claims_firmware(self):
        with tempfile.NamedTemporaryFile(delete=False) as f: f.write(rnd(100000, 11)); p = f.name
        try:
            rep = report.build(p); md = report.markdown(rep)
            self.assertIn("No recognised format", rep["conclusion"])
            self.assertEqual(md.lower().count("is firmware"), 0)
            self.assertIn("cannot determine whether any file is camera firmware", rep["conclusion"])
        finally: os.unlink(p)
    def test_cli_runs(self):
        p = os.path.join(BT, "mipsel-mips32-qemu", "hello_s650.elf")
        self.assertEqual(s650fw.main(["identify", p]), 0); self.assertEqual(s650fw.main(["hash", p]), 0)
        self.assertEqual(s650fw.main(["compare", p, p]), 0)

if __name__ == "__main__":
    unittest.main(verbosity=2)
