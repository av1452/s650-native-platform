// Instruction-set probe for the EXPERIMENTAL S650 target. NOT run on the camera.
//
// Purpose: a small program that, when run on ANY MIPS core (emulated today; real hardware only if a legitimate execution
// path is ever established and approved), reports which instruction classes that core executes. It writes a progress
// marker after each probe; a probe the core cannot execute raises a Reserved Instruction exception, so the last marker
// shows how far execution got. Results are compared with values computed on the host (tools/target-build/qemu-isa-matrix.py).
//
// Each probe is wrapped in .set directives so the assembler accepts it regardless of the -march used for the file.

using u32 = unsigned int;
extern "C" {
extern volatile u32 s650_test_buffer[16];
__attribute__((section(".s650_testbuf"), used)) volatile u32 s650_test_buffer[16];

// buffer[0]      progress: number of probes completed
// buffer[1..10]  probe results; buffer[15] completion marker
#define PROBE(n, body) body; s650_test_buffer[0] = (n)

__attribute__((section(".text._start"), noreturn, used)) void _start() {
    u32 r;
    const u32 x = 0xA5C3F00Fu, y = 0x00000007u;
    PROBE(0, (void)0);
    // 1: base MIPS-I arithmetic and shift
    asm volatile(".set push\n.set mips1\n.set noreorder\n sll %0,%1,4\n addu %0,%0,%2\n.set pop" : "=r"(r) : "r"(x), "r"(y));
    s650_test_buffer[1] = r; PROBE(1, (void)0);
    // 2: MIPS32 'mul' (3-operand multiply)
    asm volatile(".set push\n.set mips32\n.set noreorder\n mul %0,%1,%2\n.set pop" : "=r"(r) : "r"(x), "r"(y));
    s650_test_buffer[2] = r; PROBE(2, (void)0);
    // 3: MIPS32 'clz'
    asm volatile(".set push\n.set mips32\n.set noreorder\n clz %0,%1\n.set pop" : "=r"(r) : "r"(y));
    s650_test_buffer[3] = r; PROBE(3, (void)0);
    // 4: MIPS32 'movn'
    asm volatile(".set push\n.set mips32\n.set noreorder\n move %0,%1\n movn %0,%2,%2\n.set pop" : "=&r"(r) : "r"(x), "r"(y));
    s650_test_buffer[4] = r; PROBE(4, (void)0);
    // 5: MIPS32r2 'ext' (bit-field extract)
    asm volatile(".set push\n.set mips32r2\n.set noreorder\n ext %0,%1,4,8\n.set pop" : "=r"(r) : "r"(x));
    s650_test_buffer[5] = r; PROBE(5, (void)0);
    // 6: MIPS32r2 'ins' (bit-field insert)
    r = 0xFFFFFFFFu;
    asm volatile(".set push\n.set mips32r2\n.set noreorder\n ins %0,%1,8,8\n.set pop" : "+r"(r) : "r"(y));
    s650_test_buffer[6] = r; PROBE(6, (void)0);
    // 7: MIPS32r2 'seb' (sign-extend byte)
    asm volatile(".set push\n.set mips32r2\n.set noreorder\n seb %0,%1\n.set pop" : "=r"(r) : "r"(x));
    s650_test_buffer[7] = r; PROBE(7, (void)0);
    // 8: MIPS32r2 'wsbh' (word swap bytes within halfwords)
    asm volatile(".set push\n.set mips32r2\n.set noreorder\n wsbh %0,%1\n.set pop" : "=r"(r) : "r"(x));
    s650_test_buffer[8] = r; PROBE(8, (void)0);
    // 9: MIPS32r2 'rotr'
    asm volatile(".set push\n.set mips32r2\n.set noreorder\n rotr %0,%1,12\n.set pop" : "=r"(r) : "r"(x));
    s650_test_buffer[9] = r; PROBE(9, (void)0);
    // 10: MIPS-I unaligned load pair (lwl/lwr): absent from MIPS32r6 and from some licensed cores
    static const unsigned char bytes[8] __attribute__((aligned(4))) = {0, 0x11, 0x22, 0x33, 0x44, 0, 0, 0};
    {
        u32 v = 0;
        const unsigned char* p = bytes + 1;
#if defined(__MIPSEL__)
        asm volatile(".set push\n.set mips1\n.set noreorder\n"
                     "lwr %0,0(%1)\n lwl %0,3(%1)\n nop\n"   // little-endian pairing
                     ".set pop" : "+r"(v) : "r"(p) : "memory");
#else
        asm volatile(".set push\n.set mips1\n.set noreorder\n"
                     "lwl %0,0(%1)\n lwr %0,3(%1)\n nop\n"   // big-endian pairing
                     ".set pop" : "+r"(v) : "r"(p) : "memory");
#endif
        s650_test_buffer[10] = v;
    }
    PROBE(10, (void)0);
    s650_test_buffer[15] = 0x600DF00Du;
    for (;;) {}
}
}  // extern "C"
