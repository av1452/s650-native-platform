"""Assemble a Markdown/JSON report. Wording rules: recognised structures are listed with evidence; nothing is called 'firmware'."""
import json, os
from . import entropy, hashing, mips, signatures, strings_, elfinfo

def build(path: str, with_mips: bool = True) -> dict:
    with open(path, "rb") as f: data = f.read()
    rep = {"file": os.path.basename(path), "hashes": hashing.hashes(data)}
    rep["elf"] = elfinfo.parse(data)
    rep["entropy"] = entropy.profile(data)
    hits = signatures.scan(data)
    rep["signatures"] = hits
    rep["signature_summary"] = signatures.summarize(hits)
    labelled = strings_.label(strings_.extract(data))
    rep["interesting_strings"] = labelled[:200]
    rep["string_count"] = len(strings_.extract(data, limit=20000))
    if with_mips:
        if rep["elf"] and rep["elf"]["machine"] == "MIPS":
            # analyse only executable sections of a MIPS ELF so headers/symbol tables do not dilute the statistics
            code = b"".join(data[s["offset"]:s["offset"] + s["size"]] for s in rep["elf"]["sections"] if s["flags"] & 4 and s["type"] == 1)
            rep["mips"] = mips.analyze(code, known_code=True); rep["mips"]["scope"] = "ELF executable sections only (%d bytes)" % len(code)
        else:
            rep["mips"] = mips.analyze(data); rep["mips"]["scope"] = "whole file"
    rep["conclusion"] = conclude(rep)
    return rep

def conclude(rep) -> str:
    rec = [h for h in rep["signatures"] if h["level"] == "recognized"]
    parts = []
    if rep["elf"]:
        e = rep["elf"]
        parts.append("RECOGNISED: %s-bit %s-endian ELF (%s, machine %s%s)." % (e["class"], e["endian"], e["type"], e["machine"], ", " + e["mips_arch_flag"] if "mips_arch_flag" in e else ""))
    others = [h for h in rec if h["name"] != "ELF"]
    if others:
        parts.append("RECOGNISED embedded/other structures: " + ", ".join("%s@0x%x" % (h["name"], h["offset"]) for h in others[:12]) + (" ..." if len(others) > 12 else "") + ".")
    if not rec:
        parts.append("No recognised format (no signature passed structural validation).")
    m = rep.get("mips")
    if m and not rep["elf"]:
        parts.append(m["verdict"].get("summary", ""))
    parts.append("The toolkit cannot determine whether any file is camera firmware; that requires provenance, not pattern matching.")
    return " ".join(parts)

def markdown(rep: dict) -> str:
    L = ["# Analysis report: `%s`" % rep["file"], "", "## Hashes", "", "| field | value |", "|---|---|"]
    for k, v in rep["hashes"].items(): L.append("| %s | `%s` |" % (k, v))
    L += ["", "## Conclusion", "", rep["conclusion"], "", "## Recognised and possible structures", ""]
    if rep["signatures"]:
        L += ["| offset | structure | confidence | detail |", "|---|---|---|---|"]
        for h in rep["signatures"][:300]:
            L.append("| 0x%x | %s | **%s** | %s |" % (h["offset"], h["name"], h["level"], ", ".join("%s=%s" % kv for kv in h["detail"].items())))
    else:
        L.append("None.")
    L += ["", "## Entropy (speculative hint)", "", "Overall %.3f bits/byte. Profile: `%s`" % (rep["entropy"]["overall"], entropy.ascii_chart(rep["entropy"])), ""]
    L += ["| start | end | class |", "|---|---|---|"] + ["| 0x%x | 0x%x | %s |" % (r["start"], r["end"], r["class"]) for r in rep["entropy"]["regions"][:60]]
    if rep.get("mips"):
        m = rep["mips"]; L += ["", "## MIPS code evidence (speculative)", "", m["verdict"].get("summary", ""), ""]
        for e, c in m["pattern_scores"].items(): L.append("- %s-endian anchors: %d (jr ra %d, addiu sp- %d, sw ra %d, lw ra %d)" % (e, c["anchor"], c["jr $ra"], c["addiu $sp,$sp,-N"], c["sw $ra,N($sp)"], c["lw $ra,N($sp)"]))
        if m["decode"]:
            L += ["", "Scope: %s." % m.get("scope", "whole file"), "", "| variant | valid decode ratio | code-like windows |", "|---|---|---|"] + ["| %s | %.3f | %d |" % (k, v["valid_ratio"], v["code_like_windows"]) for k, v in m["decode"].items()]
            isa = m["verdict"].get("isa")
            if isa: L += ["", "Lowest ISA level implied by the instructions found: **%s** %s" % (isa["lower_bound"], isa["evidence"])]
    if rep["interesting_strings"]:
        L += ["", "## Interesting strings", ""] + ["- 0x%x [%s] `%s`" % (s["offset"], ",".join(s["tags"]), s["text"][:100]) for s in rep["interesting_strings"][:60]]
    L.append("")
    return "\n".join(L)

def to_json(rep: dict) -> str:
    rep = dict(rep); rep["entropy"] = {k: v for k, v in rep["entropy"].items() if k != "blocks"}
    return json.dumps(rep, indent=2, default=str)
