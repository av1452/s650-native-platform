cmake_minimum_required(VERSION 3.16)
# Separate EXPERIMENTAL project for the S650 target. It does not touch the desktop build in ../CMakeLists.txt.
project(s650_target LANGUAGES C CXX ASM)

set(S650_LOAD_ADDRESS "0x80000000" CACHE STRING
    "Link address. PLACEHOLDER ONLY: 0x80000000 is the architectural start of MIPS32 kseg0 and is NOT known to be camera RAM")
set(S650_STACKLESS ON)

add_executable(hello_s650 tests/hello_s650.cpp)
target_include_directories(hello_s650 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(hello_s650 PRIVATE -O2 -Wall -Wextra -Wpedantic)
# raw ld.lld options (the toolchain file links with ld.lld directly, not through the compiler driver)
target_link_options(hello_s650 PRIVATE
    -T ${CMAKE_SOURCE_DIR}/linker/placeholder.ld
    --defsym=S650_LOAD_ADDRESS=${S650_LOAD_ADDRESS}
    -Map=hello_s650.map --no-dynamic-linker -z norelro --build-id=none --static --nmagic)
set_target_properties(hello_s650 PROPERTIES SUFFIX ".elf")

# Larger analysis corpus (runs on an emulator only) and an instruction-set probe (never run on the camera).
foreach(prog mips_sample isa_probe)
  add_executable(${prog} tests/${prog}.cpp)
  target_compile_options(${prog} PRIVATE -O2 -Wall -Wextra)
  target_link_options(${prog} PRIVATE -T ${CMAKE_SOURCE_DIR}/linker/placeholder.ld --defsym=S650_LOAD_ADDRESS=${S650_LOAD_ADDRESS}
      -Map=${prog}.map --no-dynamic-linker -z norelro --build-id=none --static --nmagic)
  set_target_properties(${prog} PROPERTIES SUFFIX ".elf")
  add_custom_command(TARGET ${prog} POST_BUILD COMMAND ${S650_LLVM_PREFIX}/bin/llvm-objcopy -O binary $<TARGET_FILE:${prog}> ${prog}.bin)
endforeach()

# A flat binary copy for inspection and for emulator loading. A flat image is only one hypothesis about a loadable format;
# it is not a camera format (none is known).
add_custom_command(TARGET hello_s650 POST_BUILD
    COMMAND ${S650_LLVM_PREFIX}/bin/llvm-objcopy -O binary $<TARGET_FILE:hello_s650> hello_s650.bin
    COMMENT "flat binary (generic, not a camera image format)")
