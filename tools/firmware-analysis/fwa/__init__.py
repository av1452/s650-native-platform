"""s650fw: a read-only binary/firmware analysis toolkit (Python standard library + optional LLVM tools).

Design rules:
  * Never modifies its input. Opens files read-only.
  * Every finding carries a confidence level:
        recognized   magic bytes AND structural validation (checksum, size/field consistency, or successful decode) passed
        possible     magic bytes or partial structure only; may be a false positive
        speculative  statistical inference (entropy, instruction statistics); NOT a format identification
  * A recognised container or executable is never described as "camera firmware" by the toolkit: it cannot know that.
"""
__version__ = "0.1"
LEVELS = ("recognized", "possible", "speculative")
