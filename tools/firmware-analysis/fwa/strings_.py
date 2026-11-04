"""String extraction (ASCII and UTF-16LE) with simple labelling of interesting strings."""
import re

INTERESTING = [("version", re.compile(rb"(?i)\bv(?:er(?:sion)?)?[ .:]?\d+(?:\.\d+)+")), ("copyright", re.compile(rb"(?i)copyright|\(c\)")),
               ("vendor", re.compile(rb"(?i)sony|zoran|coach|hynix|qimonda|csr|mips")), ("path", re.compile(rb"(?:[A-Za-z]:)?[/\\][\w./\\-]{4,}")),
               ("error/log", re.compile(rb"(?i)\b(error|fail(?:ed)?|assert|panic|warning)\b")), ("url", re.compile(rb"(?i)https?://\S+"))]

def extract(data: bytes, minlen: int = 6, utf16: bool = True, limit: int = 100000):
    out = []
    for m in re.finditer(rb"[\x20-\x7e\t]{%d,}" % minlen, data):
        out.append({"offset": m.start(), "encoding": "ascii", "text": m.group().decode("ascii")})
        if len(out) >= limit: return out
    if utf16:
        for m in re.finditer(rb"(?:[\x20-\x7e]\x00){%d,}" % minlen, data):
            out.append({"offset": m.start(), "encoding": "utf-16le", "text": m.group().decode("utf-16le")})
            if len(out) >= limit: break
    out.sort(key=lambda s: s["offset"])
    return out

def label(strings):
    res = []
    for s in strings:
        b = s["text"].encode("latin1", "replace")
        tags = [name for name, rx in INTERESTING if rx.search(b)]
        if tags: res.append({**s, "tags": tags})
    return res
