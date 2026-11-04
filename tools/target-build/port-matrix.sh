#!/bin/sh
# Which parts of the real application compile for the experimental MIPS target WITHOUT a standard library?
# Compile-only (no linking): this measures source-level portability, not whether the code would run on anything.
#   A = no standard headers at all (-nostdinc++, freestanding)
#   B = LLVM libc++ headers added, but still no C library or runtime for the target. Homebrew's libc++ is configured for Apple
#       platforms and stops at its availability-markup check, so B only shows that a target-specific libc++ build would be needed.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LLVM="${LLVM_PREFIX:-/opt/homebrew/opt/llvm}"
FLAGS="--target=mipsel-none-elf -march=mips32 -mabi=32 -msoft-float -mno-abicalls -fno-pic -G0 -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -O1 -I$ROOT/include -c -o /dev/null"
printf "%-34s %-6s %-6s  %s\n" "source" "A:none" "B:hdrs" "first error with no standard library (A)"
okA=0; okB=0; n=0
for f in $(ls "$ROOT"/src/core/*.cpp "$ROOT"/src/display/*.cpp "$ROOT"/src/input/*.cpp "$ROOT"/src/camera/*.cpp "$ROOT"/src/storage/*.cpp "$ROOT"/src/ui/*.cpp | grep -v -E "StringsEn|HeadlessDisplay"); do
  n=$((n+1))
  err=$("$LLVM/bin/clang++" $FLAGS -nostdinc++ "$f" 2>&1 | grep -m1 -E "error" | sed 's/^.*error: //')
  if [ -z "$err" ]; then a=OK; okA=$((okA+1)); else a=fail; fi
  if "$LLVM/bin/clang++" $FLAGS -nostdinc++ -isystem "$LLVM/include/c++/v1" "$f" > /dev/null 2>&1; then b=OK; okB=$((okB+1)); else b=fail; fi
  printf "%-34s %-6s %-6s  %s\n" "${f#$ROOT/}" "$a" "$b" "$err"
done
echo "compiled: A $okA/$n, B $okB/$n"
