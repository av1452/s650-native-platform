/*
 * PLACEHOLDER linker script for the EXPERIMENTAL S650 target. NOT a camera memory map.
 *
 * No source read describes the S650's memory map, boot address, RAM size, or load mechanism. The only address used is
 * S650_LOAD_ADDRESS, supplied on the command line and defaulting to 0x80000000 (the architectural start of kseg0 on a
 * standard MIPS32 core). It exists so that the binary can be analysed statically and run on an EMULATED generic core;
 * it must never be interpreted as evidence about the camera.
 *
 * The program uses no stack, heap, .bss clearing, or runtime initialisation, because none of those are known to exist.
 */
ENTRY(_start)
SECTIONS
{
    . = S650_LOAD_ADDRESS;
    .text : { *(.text._start) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .s650_testbuf (NOLOAD) : ALIGN(16) { *(.s650_testbuf) }
    /DISCARD/ : { *(.comment) *(.MIPS.abiflags) *(.reginfo) *(.gnu.attributes) *(.pdr) *(.mdebug*) *(.eh_frame*) *(.note*) }
}
