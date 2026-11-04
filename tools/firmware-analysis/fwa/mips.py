"""MIPS32 code detection: byte-order scoring, decode-validity under several triples/CPUs (via llvm-mc), and an ISA lower bound.

All results are SPECULATIVE statistics unless the input is a recognised ELF (see signatures/elfinfo). They support, but never
prove, a claim about a target CPU. MIPS16e / microMIPS / MIPS64 are not modelled. Requires llvm-mc for the decode step.
"""
import os, re, shutil, struct, subprocess
from collections import Counter

LLVM = os.environ.get("LLVM_PREFIX", "/opt/homebrew/opt/llvm")

# instruction classes beyond the MIPS-I base. A mnemonic found => the code needs AT LEAST that level (a lower bound).
MIPS2_PLUS = {"beql", "bnel", "blezl", "bgtzl", "bltzl", "bgezl", "bltzall", "bgezall", "ll", "sc", "sync", "teq", "teqi", "tge", "tgei", "tgeiu", "tgeu",
              "tlt", "tlti", "tltiu", "tltu", "tne", "tnei"}
MIPS32_R1 = {"mul", "madd", "maddu", "msub", "msubu", "clz", "clo", "movn", "movz", "pref", "cache", "sdbbp", "ssnop", "wait", "tlbp", "tlbr", "tlbwi", "tlbwr", "eret"}
MIPS32_R2 = {"ext", "ins", "seb", "seh", "wsbh", "rotr", "rotrv", "jr.hb", "jalr.hb", "ehb", "di", "ei", "rdhwr", "synci", "mfhc0", "mthc0", "mfhc1", "mthc1", "rdpgpr", "wrpgpr"}
MIPS_R6_ONLY = {"jic", "jialc", "bc", "balc", "lsa", "aui", "auipc", "aluipc", "mulu", "muhu", "divu", "modu", "bovc", "bnvc", "beqzc", "bnezc", "dahi", "dati"}
UNALIGNED = {"lwl", "lwr", "swl", "swr"}
VARIANTS = [("mipsel-none-elf", "little"), ("mips-none-elf", "big")]
VARIANTS_BY_ENDIAN = [("little", "mipsel-none-elf"), ("big", "mips-none-elf")]
CPUS = ["mips1", "mips32", "mips32r2"]

def have_mc():
    return os.path.exists(f"{LLVM}/bin/llvm-mc") or shutil.which("llvm-mc") is not None

def _mc():
    return f"{LLVM}/bin/llvm-mc" if os.path.exists(f"{LLVM}/bin/llvm-mc") else "llvm-mc"

# ---- pattern scoring (no external tools) ----
def _words(data: bytes, endian: str):
    n = len(data) // 4
    return struct.unpack((">" if endian == "big" else "<") + "%dI" % n, data[:n * 4])

PATTERNS = {"jr $ra": lambda w: w == 0x03E00008, "addiu $sp,$sp,-N": lambda w: (w >> 16) == 0x27BD and (w & 0x8000) != 0,
            "sw $ra,N($sp)": lambda w: (w >> 16) == 0xAFBF, "lw $ra,N($sp)": lambda w: (w >> 16) == 0x8FBF,
            "addiu $sp,$sp,+N": lambda w: (w >> 16) == 0x27BD and (w & 0x8000) == 0, "lui": lambda w: (w >> 26) == 0x0F, "jal": lambda w: (w >> 26) == 3}

def pattern_scores(data: bytes):
    out = {}
    for e in ("little", "big"):
        ws = _words(data, e)
        c = {name: sum(1 for w in ws if f(w)) for name, f in PATTERNS.items()}
        c["nop(0)"] = sum(1 for w in ws if w == 0)
        c["words"] = len(ws)
        c["anchor"] = c["jr $ra"] + c["addiu $sp,$sp,-N"] + c["sw $ra,N($sp)"] + c["lw $ra,N($sp)"]
        out[e] = c
    return out

# ---- decode via llvm-mc (per-word validity, so code-like windows can be located inside mixed data) ----
WEAK = {"ehb", "ssnop", "nop"}          # encodings (0x000000C0, 0x00000040, 0) that small constants and padding decode to
WINDOW_WORDS = 32

def decode_words(data: bytes, triple: str, cpu: str, max_bytes: int = 262144):
    """Returns a list with one entry per aligned 32-bit word: the mnemonic, or None if llvm-mc rejects the encoding."""
    data = data[: (min(len(data), max_bytes) // 4) * 4]
    n = len(data) // 4
    if not n: return []
    text = "\n".join(" ".join("0x%02x" % b for b in data[i:i + 4]) for i in range(0, len(data), 4)) + "\n"
    r = subprocess.run([_mc(), "--disassemble", f"--triple={triple}", f"-mcpu={cpu}"], input=text, capture_output=True, text=True)
    bad = {int(m.group(1)) - 1 for m in re.finditer(r"<stdin>:(\d+):\d+: warning: invalid instruction encoding", r.stderr)}
    mnem = []
    for line in r.stdout.splitlines():
        line = line.strip()
        if line and not line.startswith("."): mnem.append(line.split()[0])
    out, k = [], 0
    for i in range(n):
        if i in bad: out.append(None)
        elif k < len(mnem): out.append(mnem[k]); k += 1
        else: out.append(None)
    return out

def code_windows(words):
    """Indices of 32-word windows that decode >=90% valid with >=4 distinct non-weak mnemonics (i.e. look like real code)."""
    res = []
    for w0 in range(0, len(words) - WINDOW_WORDS + 1, WINDOW_WORDS):
        win = words[w0:w0 + WINDOW_WORDS]
        valid = [m for m in win if m]
        distinct = {m for m in valid if m not in WEAK}
        if len(valid) >= 0.9 * WINDOW_WORDS and len(distinct) >= 4: res.append(w0)
    return res

def isa_lower_bound(mnemonics):
    m = {x for x in mnemonics if x not in WEAK}
    need = [("mips32r6 (R6-only forms present)", m & MIPS_R6_ONLY), ("mips32r2", m & MIPS32_R2), ("mips32", m & MIPS32_R1), ("mips2+", m & MIPS2_PLUS)]
    for name, found in need:
        if found: return {"lower_bound": name, "evidence": sorted(found)}
    return {"lower_bound": "mips1 (no instruction beyond MIPS-I found in code-like windows)", "evidence": []}

def analyze(data: bytes, offset: int = 0, length: int = 65536, known_code: bool = False):
    """Returns a dict of evidence plus a deliberately cautious verdict.
    known_code=True (e.g. the executable sections of a MIPS ELF) lets the ISA bound use every decodable word instead of only code-like windows."""
    chunk = data[offset: offset + length]
    chunk = chunk[(-offset % 4):] if offset % 4 else chunk
    res = {"offset": offset, "bytes_examined": len(chunk), "pattern_scores": pattern_scores(chunk), "decode": {}, "verdict": {}}
    be, le = res["pattern_scores"]["big"]["anchor"], res["pattern_scores"]["little"]["anchor"]
    pat = {"endianness": "undetermined", "confidence": "insufficient evidence", "anchors": {"big": be, "little": le}}
    if be + le >= 8:
        hi, lo = max(be, le), min(be, le); ratio = hi / max(lo, 1)
        if ratio >= 5: pat.update(endianness="big" if be > le else "little", confidence="strong pattern preference (anchor ratio %.1f)" % ratio)
        elif ratio >= 2: pat.update(endianness="big" if be > le else "little", confidence="weak pattern preference (anchor ratio %.1f)" % ratio)
        else: pat.update(confidence="ambiguous (anchor ratio %.1f)" % ratio)
    res["verdict"]["pattern_endianness"] = pat
    if not (have_mc() and len(chunk) >= 64):
        res["verdict"]["summary"] = "Decode step skipped (llvm-mc not found, or fewer than 64 bytes). Pattern evidence only: " + pat["confidence"] + "."
        return res
    arrays = {}
    for triple, e in VARIANTS:
        for cpu in CPUS:
            w = decode_words(chunk, triple, cpu)
            cw = code_windows(w)
            valid = sum(1 for x in w if x)
            mn = Counter(m for i in cw for m in w[i:i + WINDOW_WORDS] if m)
            arrays[(e, cpu)] = (w, cw)
            res["decode"][f"{e}/{cpu}"] = {"words": len(w), "valid_ratio": round(valid / max(len(w), 1), 4), "code_like_windows": len(cw),
                                           "code_like_bytes": len(cw) * WINDOW_WORDS * 4, "mnemonics_in_code_windows": dict(mn)}
    cl = {e: max(res["decode"][f"{e}/{c}"]["code_like_windows"] for c in CPUS) for e in ("little", "big")}
    res["verdict"]["code_like_windows_by_endian"] = cl
    # Decode validity alone is NOT discriminating: the MIPS32 encoding space is dense, so other architectures' code decodes
    # "validly" too (measured: arm64 code passes). The positive decision therefore requires the instruction-pattern anchors.
    words = res["pattern_scores"]["little"]["words"]
    dens = {e: 1000.0 * res["pattern_scores"][e]["anchor"] / max(words, 1) for e in ("little", "big")}
    res["verdict"]["anchor_density_per_1000_words"] = {e: round(v, 2) for e, v in dens.items()}
    best_e = "big" if res["pattern_scores"]["big"]["anchor"] > res["pattern_scores"]["little"]["anchor"] else "little"
    other_e = "little" if best_e == "big" else "big"
    a_best, a_other = res["pattern_scores"][best_e]["anchor"], res["pattern_scores"][other_e]["anchor"]
    best_cpu = "mips32r2"      # the most permissive decoder: a stricter one would hide newer instructions and under-report the ISA bound
    code_bytes = res["decode"][f"{best_e}/{best_cpu}"]["code_like_windows"] * WINDOW_WORDS * 4
    key = f"{best_e}/{best_cpu}"
    mn_code = res["decode"][key]["mnemonics_in_code_windows"]
    if mn_code and not known_code:
        isa = isa_lower_bound(mn_code.keys()); isa["basis"] = "code-like windows"
    else:   # tiny or leaf-only inputs (e.g. a probe): fall back to every decodable word and say so
        w_all = decode_words(chunk, dict(VARIANTS_BY_ENDIAN)[best_e], best_cpu)
        isa = isa_lower_bound({m for m in w_all if m}); isa["basis"] = "all decodable words (%s)" % ("input declared to be code" if known_code else "weaker: no code-like windows")
    res["verdict"]["isa"] = isa
    res["verdict"]["unaligned_access_instructions"] = {k: mn_code.get(k, 0) for k in UNALIGNED}
    decided = (a_best >= 6 and dens[best_e] >= 3.0 and a_best >= 5 * max(a_other, 1) and code_bytes >= 128)
    res["verdict"]["looks_like_mips32_code"] = bool(decided)
    if decided:
        res["verdict"]["endianness_estimate"] = best_e
        res["verdict"]["summary"] = ("MIPS-like code, %s-endian: %d anchor patterns (%.1f per 1000 words; other byte order %d), %d bytes in code-like windows. "
                                     "SPECULATIVE statistical inference; not a format identification and not a CPU identification."
                                     % (best_e, a_best, dens[best_e], a_other, code_bytes))
    else:
        res["verdict"]["summary"] = ("No convincing evidence of 32-bit MIPS code (anchor patterns: little=%d, big=%d; code-like windows: little=%d, big=%d)."
                                     % (res["pattern_scores"]["little"]["anchor"], res["pattern_scores"]["big"]["anchor"], cl["little"], cl["big"]))
    return res
