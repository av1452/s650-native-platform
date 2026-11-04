import hashlib, zlib

def hashes(data: bytes) -> dict:
    return {"size": len(data), "sha256": hashlib.sha256(data).hexdigest(), "sha1": hashlib.sha1(data).hexdigest(),
            "md5": hashlib.md5(data).hexdigest(), "crc32": "%08x" % (zlib.crc32(data) & 0xFFFFFFFF)}
