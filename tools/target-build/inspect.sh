#!/bin/sh
# Static inspection of a target ELF: format, entry, sections, size, undefined symbols, stack use, checksum.
# Usage: inspect.sh file.elf [llvm_prefix]
set -u
ELF="$1"; LLVM="${2:-/opt/homebrew/opt/llvm}"
echo "== file"; ls -l "$ELF" | awk '{print $5" bytes  "$9}'
BIN="${ELF%.elf}.bin"; [ -f "$BIN" ] && echo "flat binary: $(wc -c < "$BIN" | tr -d ' ') bytes, sha256 $(shasum -a 256 "$BIN" | cut -d' ' -f1)"
echo "sha256: $(shasum -a 256 "$ELF" | cut -d' ' -f1)"
echo "== ELF header"; "$LLVM/bin/llvm-readelf" -h "$ELF" | grep -E "Class|Data|Machine|Flags|Entry|Type"
echo "== sections"; "$LLVM/bin/llvm-readelf" -S "$ELF" | sed -n '1,40p'
echo "== program headers"; "$LLVM/bin/llvm-readelf" -l "$ELF" | sed -n '1,20p'
echo "== size"; "$LLVM/bin/llvm-size" "$ELF"
printf "size_total: %s bytes (file)\n" "$(wc -c < "$ELF" | tr -d ' ')"
echo "== symbols"; "$LLVM/bin/llvm-nm" -n "$ELF"
UND=$("$LLVM/bin/llvm-nm" -u "$ELF" | wc -l | tr -d ' '); echo "undefined_symbols: $UND"
echo "== disassembly"; "$LLVM/bin/llvm-objdump" -d --no-show-raw-insn "$ELF"
DIS=$("$LLVM/bin/llvm-objdump" -d --no-show-raw-insn "$ELF")
if echo "$DIS" | grep -q -E '\$sp|\$29|\$fp|\$30'; then echo "stack_or_frame_register_use: YES (unexpected)"; else echo "stack_or_frame_register_use: none"; fi
if echo "$DIS" | grep -q -E 'jal|jalr|bal'; then echo "calls: present"; else echo "calls: none"; fi
echo "program_id: $(strings -a "$ELF" | grep S650-HELLO | head -1)"
