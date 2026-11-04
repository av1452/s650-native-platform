// Analysis corpus and codegen check for the EXPERIMENTAL S650 target (not camera code).
//
// A larger freestanding program than hello_s650: loops, recursion, a jump table, function pointers, an unaligned read,
// 64-bit multiplication and 32-bit division. It is used (a) to compare the instructions the compiler emits for each
// endianness / ISA hypothesis, (b) as a realistic sample for the firmware-analysis toolkit, and (c) to check codegen
// correctness on an EMULATED generic MIPS core by comparing results with values computed independently on the host.
// The stack lives inside this image (a static array), so no external memory address is assumed.

using u8 = unsigned char;
using u32 = unsigned int;
using u64 = unsigned long long;
static_assert(sizeof(u32) == 4 && sizeof(u64) == 8, "o32 data model expected");

extern "C" {

extern volatile u32 s650_test_buffer[16];
__attribute__((section(".s650_testbuf"), used)) volatile u32 s650_test_buffer[16];

__attribute__((aligned(16), used)) static u8 s650_stack[2048];

static u32 lcg(u32& s) { s = s * 1664525u + 1013904223u; return s; }

static u32 crc32_bitwise(const u8* p, u32 n) {
    u32 c = 0xFFFFFFFFu;
    for (u32 i = 0; i < n; ++i) {
        c ^= p[i];
        for (int k = 0; k < 8; ++k) c = (c & 1u) ? (c >> 1) ^ 0xEDB88320u : (c >> 1);
    }
    return ~c;
}

static void insertion_sort(u32* a, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        u32 v = a[i], j = i;
        while (j > 0 && a[j - 1] > v) { a[j] = a[j - 1]; --j; }
        a[j] = v;
    }
}

static u32 fib(u32 n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

static u32 jump_table(u32 x) {
    switch (x & 7u) {
        case 0: return x + 11; case 1: return x * 3; case 2: return x ^ 0x5A5Au; case 3: return x - 7;
        case 4: return x << 2; case 5: return x >> 1; case 6: return ~x; default: return x | 0x100u;
    }
}

static u32 add1(u32 x) { return x + 1u; }
static u32 dbl(u32 x) { return x * 2u; }
static u32 neg(u32 x) { return 0u - x; }
typedef u32 (*Fn)(u32);
static const Fn kFns[3] = {add1, dbl, neg};

struct __attribute__((packed)) Packed { u8 pad; u32 value; };

static u32 read_unaligned(const u8* base) { return reinterpret_cast<const Packed*>(base)->value; }

static void run() {
    u32 seed = 12345u;
    u32 data[64];
    for (int i = 0; i < 64; ++i) data[i] = lcg(seed) >> 8;
    insertion_sort(data, 64);
    u32 sorted_sum = 0;
    for (u32 i = 0; i < 64; ++i) sorted_sum += (i + 1u) * data[i];

    u8 bytes[256];
    seed = 777u;
    for (int i = 0; i < 256; ++i) bytes[i] = static_cast<u8>(lcg(seed) >> 24);

    u32 jt = 0;
    for (u32 i = 0; i < 32; ++i) jt += jump_table(i * 2654435761u);
    u32 fp = 5;
    for (int i = 0; i < 9; ++i) fp = kFns[i % 3](fp);

    u8 raw[8] = {0x01, 0x78, 0x56, 0x34, 0x12, 0, 0, 0};     // value bytes 78 56 34 12 at offset 1
    u32 un = read_unaligned(raw);

    u32 a = 0xDEADBEEFu, b = 0x12345678u;
    u32 mulhi = static_cast<u32>((static_cast<u64>(a) * b) >> 32);
    u32 vdiv = a / (b | 1u), vmod = a % (b | 1u);

    s650_test_buffer[0] = sorted_sum;
    s650_test_buffer[1] = crc32_bitwise(bytes, 256);
    s650_test_buffer[2] = fib(20);
    s650_test_buffer[3] = jt;
    s650_test_buffer[4] = fp;
    s650_test_buffer[5] = un;                  // endian-independent: bytes are assembled by the CPU's load, so this also
                                               // reveals whether unaligned access instructions work as the ISA says
    s650_test_buffer[6] = mulhi;
    s650_test_buffer[7] = vdiv ^ vmod;
    s650_test_buffer[15] = 0x600DF00Du;        // completion marker
}

// Entry: set the stack to the in-image array, call run(), spin. No other runtime.
__attribute__((section(".text._start"), noreturn, naked, used)) void _start() {
    asm volatile(
        ".set push\n.set noreorder\n"
        "la $sp, s650_stack + 2048\n"
        "jal %0\n nop\n"
        "1: b 1b\n nop\n"
        ".set pop\n" ::"i"(run) : );
}

}  // extern "C"
