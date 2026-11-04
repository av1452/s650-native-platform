"""Shannon entropy per block and a coarse classification. Entropy cannot distinguish compression from encryption."""
import math
from collections import Counter

def shannon(block: bytes) -> float:
    if not block: return 0.0
    n = len(block); c = Counter(block)
    return -sum((v / n) * math.log2(v / n) for v in c.values())

def classify(h: float, block: bytes = b"") -> str:
    if block and len(set(block)) == 1: return "constant"
    if h < 0.5: return "near-constant"
    if h < 4.5: return "structured (text/tables/sparse code)"
    if h < 7.2: return "dense (code/data mix)"
    return "high-entropy (compressed OR encrypted; cannot tell)"

def profile(data: bytes, block: int = 4096) -> dict:
    blocks = []
    for off in range(0, len(data), block):
        b = data[off:off + block]
        h = shannon(b)
        blocks.append({"offset": off, "entropy": round(h, 3), "class": classify(h, b)})
    regions = []
    for b in blocks:
        if regions and regions[-1]["class"] == b["class"]:
            regions[-1]["end"] = b["offset"] + block; regions[-1]["blocks"] += 1
        else:
            regions.append({"start": b["offset"], "end": b["offset"] + block, "class": b["class"], "blocks": 1})
    for r in regions: r["end"] = min(r["end"], len(data))
    return {"overall": round(shannon(data), 3), "block_size": block, "blocks": blocks, "regions": regions,
            "note": "Entropy is a statistical hint (speculative), not a format identification."}

def ascii_chart(prof: dict, width: int = 64) -> str:
    bl = prof["blocks"]
    if not bl: return ""
    step = max(1, len(bl) // width)
    chars = " .:-=+*#%@"
    out = []
    for i in range(0, len(bl), step):
        h = sum(b["entropy"] for b in bl[i:i + step]) / len(bl[i:i + step])
        out.append(chars[min(9, int(h / 8 * 9.99))])
    return "".join(out)
