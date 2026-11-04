#!/usr/bin/env python3
"""Measures what actually prevents each application source file from compiling for the freestanding MIPS target.

1. Static scan: std:: facilities, standard headers, floating point, heap and exception use per file (direct, and via project headers).
2. Compile probes (clang, target mipsel, freestanding, compile-only): with no standard headers at all, and with minimal shim headers
   (target/shim: cstdint, cstddef, cstring, cmath, algorithm map to compiler builtins) - recording the first error.
3. Greedy unblock plan: which facility, if provided, would let the most files compile.
Output: docs/generated/portability-measure.md. No camera knowledge is involved.
"""
import collections, os, re, subprocess, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LLVM = os.environ.get("LLVM_PREFIX", "/opt/homebrew/opt/llvm")
SRC_DIRS = ["core", "display", "input", "camera", "storage", "ui"]
EXCLUDE = {"StringsEn.cpp", "HeadlessDisplay.cpp"}
FLAGS = ["--target=mipsel-none-elf", "-march=mips32", "-mabi=32", "-msoft-float", "-mno-abicalls", "-fno-pic", "-G0", "-ffreestanding",
         "-fno-exceptions", "-fno-rtti", "-std=c++17", "-O1", "-c", "-o", "/dev/null", "-I" + ROOT + "/include", "-nostdinc++"]

def sources():
    out = []
    for d in SRC_DIRS:
        for f in sorted(os.listdir(f"{ROOT}/src/{d}")):
            if f.endswith(".cpp") and f not in EXCLUDE: out.append(f"src/{d}/{f}")
    return out

def read(p):
    with open(os.path.join(ROOT, p), errors="replace") as f: return f.read()

def includes(text):
    return re.findall(r'#\s*include\s*([<"])([^>"]+)[>"]', text)

def closure(path, seen=None):
    """Project headers reachable from a file (quoted/angle includes that exist under include/ or next to the file)."""
    seen = seen if seen is not None else set()
    text = read(path)
    for _, name in includes(text):
        for cand in (f"include/{name}", os.path.join(os.path.dirname(path), name)):
            if os.path.exists(os.path.join(ROOT, cand)) and cand not in seen:
                seen.add(cand); closure(cand, seen)
    return seen

FACILITIES = {
    "std::string": r"std::string\b", "std::vector": r"std::vector\b", "std::function": r"std::function\b", "std::shared_ptr/unique_ptr": r"std::(shared_ptr|unique_ptr|make_shared|make_unique)\b",
    "std::unordered_map": r"std::unordered_map\b", "std::pair/tie": r"std::(pair|make_pair|tie)\b", "std::deque": r"std::deque\b", "iostream/sstream/fstream": r"std::(i|o)?(string)?stream|std::ifstream|std::ofstream|std::stringstream",
    "std::chrono": r"std::chrono", "std::min/max/clamp/sort": r"std::(min|max|clamp|sort|swap|reverse)\b", "math (std::sin, pow, ...)": r"std::(sin|cos|pow|exp|log|log2|log10|sqrt|floor|ceil|fabs|hypot|atan2)\b",
    "snprintf/stol (C/C++ library)": r"\b(snprintf|std::stol|std::to_string|std::stoi|rename|localtime_r|gmtime_r|timegm|time)\s*\(",
    "std::initializer_list": r"std::initializer_list", "exceptions": r"\b(try|catch|throw)\b", "heap (new/delete)": r"\b(new|delete)\b\s*[A-Za-z\[(]"}

def scan(path):
    text = read(path)
    # include closure union
    files = {path} | closure(path)
    blob = "\n".join(read(f) for f in files)
    direct = {name: len(re.findall(rx, text)) for name, rx in FACILITIES.items()}
    trans = {name: len(re.findall(rx, blob)) for name, rx in FACILITIES.items()}
    std_headers = sorted({n for f in files for c, n in includes(read(f)) if c == "<"})
    flt = len(re.findall(r"\b(float|double)\b", text))
    return {"direct": {k: v for k, v in direct.items() if v}, "transitive": {k: v for k, v in trans.items() if v}, "std_headers": std_headers, "float_mentions": flt, "lines": text.count("\n")}

def probe(path, shim):
    cmd = [f"{LLVM}/bin/clang++"] + FLAGS + ([f"-isystem{ROOT}/target/shim"] if shim else []) + [os.path.join(ROOT, path)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    err = next((l for l in r.stderr.splitlines() if "error:" in l), "")
    return r.returncode == 0, re.sub(r"^.*error: ", "", err)

def main():
    rows = []
    for p in sources():
        s = scan(p); a = probe(p, False); b = probe(p, True)
        rows.append((p, s, a, b))
    L = ["# Generated: embedded portability measurement", "", "Produced by `tools/portability/measure.py`. Compile probes: clang, `--target=mipsel-none-elf -march=mips32 -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -nostdinc++`, compile-only.", "",
         "| source | lines | std headers (transitive) | blocked by (no std headers) | with minimal shims (`target/shim`) |", "|---|---|---|---|---|"]
    for p, s, a, b in rows:
        L.append(f"| `{p}` | {s['lines']} | {', '.join(s['std_headers']) or '-'} | {'**compiles**' if a[0] else a[1][:70]} | {'**compiles**' if b[0] else b[1][:70]} |")
    okA = sum(1 for r in rows if r[2][0]); okB = sum(1 for r in rows if r[3][0])
    L += ["", f"**Compile result:** {okA}/{len(rows)} without any standard headers; {okB}/{len(rows)} with the minimal shims.", ""]
    fac = collections.Counter(); filesby = collections.defaultdict(set)
    for p, s, a, b in rows:
        for k in s["transitive"]: fac[k] += 1; filesby[k].add(p)
    L += ["## Facility usage (files affected, including via project headers)", "", "| facility | files affected | direct uses in .cpp files |", "|---|---|---|"]
    direct = collections.Counter()
    for p, s, a, b in rows:
        for k, v in s["direct"].items(): direct[k] += v
    for k, v in fac.most_common(): L.append(f"| {k} | {v} | {direct.get(k, 0)} |")
    # greedy unblock plan
    need = {p: set(s["transitive"]) - {"std::min/max/clamp/sort", "math (std::sin, pow, ...)", "heap (new/delete)", "std::initializer_list", "exceptions"} for p, s, a, b in rows}
    provided, plan = set(), []
    remaining = dict(need)
    while True:
        done = [p for p, n in remaining.items() if n <= provided]
        cand = collections.Counter()
        for p, n in remaining.items():
            for f in n - provided: cand[f] += 1
        if not cand: break
        best = max(cand, key=lambda f: (sum(1 for p, n in remaining.items() if (n - provided) <= {f}), -len(filesby[f])))
        provided.add(best)
        plan.append((best, sum(1 for p, n in need.items() if n <= provided)))
        if len(plan) > 12: break
    L += ["", "## Greedy unblock plan (provide facilities one at a time; counts are files whose standard-library needs are then fully met)", "", "| step | provide | files unblocked (cumulative) |", "|---|---|---|"]
    for i, (f, n) in enumerate(plan, 1): L.append(f"| {i} | {f} | {n}/{len(rows)} |")
    L += ["", "(math and algorithm helpers, heap use and initializer_list are excluded from the plan: they map to compiler builtins / need no library at compile time, but linking needs compiler-rt and a libm.)", ""]
    out = os.path.join(ROOT, "docs", "generated"); os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "portability-measure.md"), "w") as f: f.write("\n".join(L))
    print("\n".join(L))

if __name__ == "__main__": main()
