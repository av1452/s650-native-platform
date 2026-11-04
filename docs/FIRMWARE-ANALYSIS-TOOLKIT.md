# Firmware analysis toolkit (`tools/firmware-analysis/`)

A read-only, dependency-light toolkit for analysing binaries that may one day include original camera firmware. **We have no S650 firmware**, so the toolkit is validated on our own binaries, synthetic fixtures and negative controls only. It never modifies its input.

## Design rules
1. **Three confidence levels on every finding:** `recognized` (magic *and* structural validation: checksum, size/field consistency, or a successful decode), `possible` (magic or partial structure only), `speculative` (statistics such as entropy or instruction patterns). Statistical findings are never called a format.
2. **It cannot say "this is camera firmware".** Reports state this explicitly; provenance, not pattern matching, decides that.
3. **Use existing tools where they exist:** MIPS decoding is delegated to `llvm-mc` (we did not write a disassembler); Ghidra is the recommended interactive tool once an image exists (not installed). Python standard library only for everything else.
4. Weak magics (2-byte zlib/MZ/BM headers) are reported only when validation succeeds.

## Use
```sh
tools/firmware-analysis/s650fw.py hash FILE
tools/firmware-analysis/s650fw.py identify FILE          # signatures with confidence levels
tools/firmware-analysis/s650fw.py entropy FILE [--block 4096]
tools/firmware-analysis/s650fw.py strings FILE [--interesting] [--min 6]
tools/firmware-analysis/s650fw.py elf FILE
tools/firmware-analysis/s650fw.py mips FILE [--offset 0x1000 --length 0x20000]
tools/firmware-analysis/s650fw.py compare A B
tools/firmware-analysis/s650fw.py report FILE [-o report.md] [--json] [--no-mips]
python3 tools/firmware-analysis/tests/test_toolkit.py     # 20 tests
```
`LLVM_PREFIX` selects the LLVM install (default Homebrew). Without `llvm-mc` the MIPS decode step is skipped and the report says so.

## Modules (`fwa/`)
| Module | Function | Notes |
|---|---|---|
| `hashing` | size, SHA-256, SHA-1, MD5, CRC-32 | |
| `entropy` | per-block Shannon entropy, region classes, ASCII profile | A hint only: compressed and encrypted data look alike |
| `elfinfo` | pure-Python ELF32/64 reader (class, endianness, machine, flags, sections) | MIPS arch/ABI flags decoded |
| `signatures` | scan with validators (below) | |
| `strings_` | ASCII and UTF-16LE extraction; tags (version, vendor, path, error, URL, copyright) | |
| `mips` | byte-order pattern scoring, per-word decode via `llvm-mc` under 6 triple/CPU combinations, code-like windows, ISA lower bound | speculative |
| `compare` | identity, common prefix/suffix, differing 512-byte regions, block-hash overlap | detects moves on the 512-byte grid only (documented limitation) |
| `report` | Markdown/JSON report with a conservative conclusion | |

## Recognised formats and what "recognized" requires
| Format | Validation beyond the magic |
|---|---|
| ELF | class/endianness/version/header-size consistent; machine and MIPS flags decoded |
| gzip, zlib, bzip2 | header fields valid **and** the stream decodes (zlib additionally needs >= 16 output bytes or a complete stream) |
| xz | stream-flags CRC-32 |
| PNG | IHDR CRC-32 |
| JPEG | marker structure and an end-of-image marker found |
| BMP, RIFF (AVI/WAVE/WEBP), ZIP, tar (ustar), 7-Zip | field consistency / header checksum |
| U-Boot uImage | header CRC-32 (reports arch, load/entry addresses) |
| Device tree (FDT) | sizes/offsets/version plausibility |
| SquashFS, cramfs, romfs | superblock field checks (block size power of two, "Compressed ROMFS" string) |
| FAT12/16/32 boot sector | BPB sanity at 512-byte-aligned offsets and 0x55AA signature (relevant: the camera's USB volume is FAT12) |
| ext2/3/4, ISO 9660, PE | superblock/descriptor fields |
| UBI, Mach-O | `possible` only (checksum not verified / magic only) |
Not detected: JFFS2 (2-byte magic too weak), YAFFS, vendor-specific containers, MIPS16/microMIPS code, MIPS64, other architectures.

## Verification (what the 20 tests establish)
- **Ground truth on our own binaries:** ELF headers of all 12 variants match their names (byte order, ABI, ISA flag); raw `.text` of `mips_sample` in 6 endian/ISA combinations -> byte order recovered **6/6**; ISA lower bound never exceeds the declared `-march`; the probe's R2 instructions yield `mips32r2`.
- **Synthetic fixtures:** gzip/zlib/bzip2/xz, PNG, JPEG, BMP, AVI, ZIP, tar, uImage, FDT, SquashFS, FAT boot sector, ext superblock are `recognized` at the expected offsets, even when embedded after padding.
- **Downgrade behaviour:** a gzip header with a garbage body and a uImage with a wrong header CRC are `possible`, not `recognized`; a PNG with a bad CRC is `possible`.
- **False-positive controls:** 2 MB of random data and 2 MB of text yield **zero** recognised structures.
- **Foreign-architecture control:** arm64 machine code from the desktop build is **not** reported as MIPS. *(Before the fix, it was: decode validity is a weak signal in the dense MIPS32 encoding space; positive claims now require `jr ra` / stack-frame anchor patterns at >= 3 per 1000 words and a >= 5x preference for one byte order.)*
- **Report wording:** a random file's report says "No recognised format" and the disclaimer; it never says "is firmware".

## Known limitations
- Code detection needs enough code: small inputs (< a few hundred bytes) and leaf-only code with no `jr ra`/stack frames are `undetermined` (e.g. the ISA probe).
- Entropy cannot separate compression from encryption. Scans are exhaustive but offset-aligned checks (FAT) only at 512-byte boundaries.
- The block-comparison grid is 512 bytes. A 100-byte insertion hides shared content (tested and documented).
- The toolkit has never seen real camera firmware; thresholds were tuned on our own MIPS output and a handful of controls, so real firmware (denser, mixed with data and compressed regions) may need retuning. That would be recorded, not assumed.
