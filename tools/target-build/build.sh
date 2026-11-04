#!/bin/sh
# Builds the EXPERIMENTAL S650 target program for every endianness/ISA hypothesis, then inspects each result.
# Nothing here targets verified camera hardware. See docs/CROSS-COMPILATION.md.
#   tools/target-build/build.sh            # all variants, output in build-target/
#   LLVM_PREFIX=/path tools/target-build/build.sh
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/build-target"
LLVM="${LLVM_PREFIX:-/opt/homebrew/opt/llvm}"
[ -x "$LLVM/bin/clang" ] || { echo "LLVM not found at $LLVM (brew install llvm lld, or set LLVM_PREFIX)"; exit 1; }
mkdir -p "$OUT"
: > "$OUT/summary.txt"
for endian in little big; do
  for isa in mips1 mips32 mips32r2; do
    for kind in placeholder qemu; do
      if [ "$kind" = qemu ]; then addr=0x80100000; else addr=0x80000000; fi
      [ "$endian" = little ] && t=mipsel || t=mips
      name="$t-$isa-$kind"
      dir="$OUT/$name"
      rm -rf "$dir"
      cmake -S "$ROOT/target" -B "$dir" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/s650-target.cmake" -DS650_ENDIAN=$endian -DS650_ISA=$isa \
            -DS650_LLVM_PREFIX="$LLVM" -DS650_LOAD_ADDRESS=$addr -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > "$dir.configure.log" 2>&1 || { echo "configure failed: $name (see $dir.configure.log)"; exit 1; }
      cmake --build "$dir" > "$dir.build.log" 2>&1 || { echo "build failed: $name (see $dir.build.log)"; exit 1; }
      "$ROOT/tools/target-build/inspect.sh" "$dir/hello_s650.elf" "$LLVM" > "$dir/inspection.txt" 2>&1 || true
      {
        echo "variant:        $name"
        echo "compiler:       $("$LLVM/bin/clang" --version | head -1)"
        echo "link address:   $addr ($( [ "$kind" = qemu ] && echo 'QEMU-malta validation address' || echo 'PLACEHOLDER, not camera memory'))"
        echo "compile command (from compile_commands.json):"
        python3 -I -c "import json,sys; print(json.load(open('$dir/compile_commands.json'))[0]['command'])"
        echo "status: EXPERIMENTAL, UNVERIFIED on camera hardware"
      } > "$dir/build-info.txt"
      printf "%-26s %s\n" "$name" "$(grep -E '^size_total' "$dir/inspection.txt" | head -1)" >> "$OUT/summary.txt"
    done
  done
done
cat "$OUT/summary.txt"
