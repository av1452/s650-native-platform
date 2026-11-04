#!/usr/bin/env python3
"""s650fw - read-only firmware/binary analysis toolkit. See docs/FIRMWARE-ANALYSIS-TOOLKIT.md."""
import argparse, json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fwa import compare, entropy, hashing, mips, report, signatures, strings_, elfinfo

def load(p):
    with open(p, "rb") as f: return f.read()

def main(argv=None):
    ap = argparse.ArgumentParser(prog="s650fw", description="Read-only binary analysis (never modifies input).")
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("hash", "identify", "scan", "entropy", "strings", "elf"):
        sp = sub.add_parser(name); sp.add_argument("file")
        if name == "entropy": sp.add_argument("--block", type=int, default=4096)
        if name == "strings": sp.add_argument("--min", type=int, default=6); sp.add_argument("--interesting", action="store_true")
    sp = sub.add_parser("mips"); sp.add_argument("file"); sp.add_argument("--offset", type=lambda x: int(x, 0), default=0); sp.add_argument("--length", type=lambda x: int(x, 0), default=65536)
    sp = sub.add_parser("compare"); sp.add_argument("a"); sp.add_argument("b")
    sp = sub.add_parser("report"); sp.add_argument("file"); sp.add_argument("-o"); sp.add_argument("--json", action="store_true"); sp.add_argument("--no-mips", action="store_true")
    a = ap.parse_args(argv)
    if a.cmd == "hash": print(json.dumps(hashing.hashes(load(a.file)), indent=2))
    elif a.cmd == "elf": print(json.dumps(elfinfo.parse(load(a.file)), indent=2, default=str))
    elif a.cmd in ("identify", "scan"):
        d = load(a.file); hits = signatures.scan(d)
        for h in hits: print("0x%08x  %-11s %-34s %s" % (h["offset"], h["level"], h["name"], h["detail"]))
        if not hits: print("no signatures")
        print(json.dumps(signatures.summarize(hits)))
    elif a.cmd == "entropy":
        p = entropy.profile(load(a.file), a.block); print("overall %.3f  %s" % (p["overall"], entropy.ascii_chart(p)))
        for r in p["regions"]: print("0x%08x-0x%08x  %s" % (r["start"], r["end"], r["class"]))
    elif a.cmd == "strings":
        s = strings_.extract(load(a.file), a.min); s = strings_.label(s) if a.interesting else s
        for x in s: print("0x%08x %-8s %s%s" % (x["offset"], x["encoding"], x["text"], "  " + str(x["tags"]) if "tags" in x else ""))
    elif a.cmd == "mips": print(json.dumps(mips.analyze(load(a.file), a.offset, a.length), indent=2))
    elif a.cmd == "compare": print(json.dumps(compare.compare(load(a.a), load(a.b)), indent=2))
    elif a.cmd == "report":
        rep = report.build(a.file, not a.no_mips); out = report.to_json(rep) if a.json else report.markdown(rep)
        if a.o:
            with open(a.o, "w") as f: f.write(out)
        else: print(out)
    return 0

if __name__ == "__main__": sys.exit(main())
