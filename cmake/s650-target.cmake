# EXPERIMENTAL cross-compilation toolchain file for the Sony DSC-S650 (Zoran ZR36473 / COACH 9S).
#
# STATUS: UNVERIFIED. The CPU core, ISA revision, endianness and ABI of the ZR36473 are NOT established by any source
# read (see docs/TARGET-ARCHITECTURE.md). Every setting below is a clearly labelled working hypothesis chosen to be as
# conservative as possible, NOT a statement about the camera. A binary built with this file is a generic bare-metal
# MIPS ELF. It is NOT a camera-compatible program and has never been run on the camera.
#
# Usage (from the repository root):
#   cmake -S target -B build-target/mipsel-mips32 \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/s650-target.cmake -DS650_ENDIAN=little -DS650_ISA=mips32
#
# Selectable hypotheses:
#   S650_ENDIAN = little | big          (UNKNOWN for the real chip; both are built by tools/target-build/build.sh)
#   S650_ISA    = mips1 | mips32 | mips32r2   (mips1 = most conservative subset; mips32/r2 = hypothesis if the core is a 4KE-class MIPS32)
#   S650_LLVM_PREFIX = path to the LLVM install (default: Homebrew llvm)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)

if(NOT DEFINED S650_ENDIAN)
  set(S650_ENDIAN little)
endif()
if(NOT DEFINED S650_ISA)
  set(S650_ISA mips32)
endif()
if(NOT DEFINED S650_LLVM_PREFIX)
  if(EXISTS /opt/homebrew/opt/llvm/bin/clang)
    set(S650_LLVM_PREFIX /opt/homebrew/opt/llvm)
  elseif(EXISTS /usr/local/opt/llvm/bin/clang)
    set(S650_LLVM_PREFIX /usr/local/opt/llvm)
  else()
    set(S650_LLVM_PREFIX /usr)
  endif()
endif()

if(S650_ENDIAN STREQUAL "little")
  set(S650_TRIPLE mipsel-none-elf)
elseif(S650_ENDIAN STREQUAL "big")
  set(S650_TRIPLE mips-none-elf)
else()
  message(FATAL_ERROR "S650_ENDIAN must be little or big")
endif()

set(CMAKE_C_COMPILER   ${S650_LLVM_PREFIX}/bin/clang)
set(CMAKE_CXX_COMPILER ${S650_LLVM_PREFIX}/bin/clang++)
set(CMAKE_C_COMPILER_TARGET   ${S650_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${S650_TRIPLE})
set(CMAKE_ASM_COMPILER_TARGET ${S650_TRIPLE})
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)   # no linking during compiler checks: there is no runtime to link against

# Conservative, evidence-free-as-possible code generation:
#  -mabi=32 -msoft-float   no FPU assumed (4KE-class cores have none; unverified for the ZR36473)
#  -mno-abicalls -fno-pic  absolute, non-PIC code (no dynamic loader or GOT exists to rely on)
#  -G0                     no small-data section (no $gp setup assumed)
#  -mno-unaligned-access is not used: instead we simply avoid packed structs in target code
set(S650_ARCH_FLAGS "-march=${S650_ISA} -mabi=32 -msoft-float -mno-abicalls -fno-pic -G0")
set(S650_FREESTANDING_FLAGS "-ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-unwind-tables -fno-asynchronous-unwind-tables -nostdlib -fno-stack-protector")
set(CMAKE_C_FLAGS_INIT   "${S650_ARCH_FLAGS} ${S650_FREESTANDING_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${S650_ARCH_FLAGS} ${S650_FREESTANDING_FLAGS}")
# Link with ld.lld directly. Going through the clang driver on a macOS host injects Mach-O linker arguments
# (-platform_version, -syslibroot) that do not belong to a bare-metal ELF link.
if(NOT DEFINED S650_LLD)
  find_program(S650_LLD ld.lld HINTS /opt/homebrew/opt/lld/bin /opt/homebrew/bin ${S650_LLVM_PREFIX}/bin /usr/local/bin)
endif()
if(NOT S650_LLD)
  message(FATAL_ERROR "ld.lld not found (brew install lld, or pass -DS650_LLD=/path/to/ld.lld)")
endif()
set(CMAKE_C_LINK_EXECUTABLE   "${S650_LLD} <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "${S650_LLD} <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
