"""Compare two binaries: identity, block-level differences, common prefix/suffix, and shifted-content detection."""
import hashlib

def compare(a: bytes, b: bytes, block: int = 512) -> dict:
    ha, hb = hashlib.sha256(a).hexdigest(), hashlib.sha256(b).hexdigest()
    n = min(len(a), len(b))
    pre = 0
    while pre < n and a[pre] == b[pre]: pre += 1
    suf = 0
    while suf < n - pre and a[-1 - suf] == b[-1 - suf]: suf += 1
    diffs = []
    for off in range(0, n, block):
        if a[off:off + block] != b[off:off + block]:
            if diffs and diffs[-1]["end"] == off: diffs[-1]["end"] = min(off + block, n)
            else: diffs.append({"start": off, "end": min(off + block, n)})
    differing = sum(d["end"] - d["start"] for d in diffs)
    # block-hash overlap catches content that moved (e.g. an inserted header shifts everything)
    ba = {hashlib.sha1(a[o:o + block]).digest() for o in range(0, len(a) - block + 1, block) if len(set(a[o:o + block])) > 1}
    bb = {hashlib.sha1(b[o:o + block]).digest() for o in range(0, len(b) - block + 1, block) if len(set(b[o:o + block])) > 1}
    shared = len(ba & bb)
    return {"identical": ha == hb, "size_a": len(a), "size_b": len(b), "sha256_a": ha, "sha256_b": hb, "common_prefix": pre, "common_suffix": suf,
            "differing_regions": diffs[:200], "differing_bytes_aligned": differing,
            "aligned_block_overlap": shared, "distinct_blocks": (len(ba), len(bb)),
            "note": "block overlap counts identical non-constant 512-byte blocks regardless of position"}
