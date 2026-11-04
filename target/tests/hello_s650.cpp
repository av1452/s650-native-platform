// Smallest possible EXPERIMENTAL target program for the S650 platform.
//
// What it is: a freestanding, bare-metal MIPS program with a recognisable identifier that writes a deterministic
// pattern into a test buffer and then spins. It uses no stack, no heap, no libc, no exceptions, no RTTI and no
// operating-system call, because none of those are known to exist on the camera.
//
// What it is NOT: a camera-compatible application. Nothing is known about how the camera would load or start it, where it
// could live, or what memory exists at its link address. It has NEVER been executed on the camera. Its only
// verification is (a) static inspection and (b) execution on an EMULATED generic MIPS core (tools/target-build/qemu-verify.py).

// No standard-library headers: none exist for this bare-metal target. The 32-bit type is checked instead of assumed.
using u32 = unsigned int;
static_assert(sizeof(u32) == 4, "the o32 ABI is expected to give a 32-bit unsigned int");

extern "C" {

// Recognisable identifier, easy to find in a hex dump or a memory image.
extern const char s650_program_id[];
const char s650_program_id[] = "S650-HELLO-0001|EXPERIMENTAL|NOT-CAMERA-VERIFIED";

// Eight 32-bit words the program fills; a verifier reads them back from memory.
// Expected final contents: [0]=0x53363530 ('S650'), [1]=0x48454C4C ('HELL'), [2..6]=0x00000001..0x00000005, [7]=0xFFFFFFFF
extern volatile u32 s650_test_buffer[8];
__attribute__((section(".s650_testbuf"), used)) volatile u32 s650_test_buffer[8];

// Entry point. A leaf function with no locals: the compiler needs no stack frame. tools/target-build/inspect.sh
// checks the disassembly to make sure no stack-pointer use slipped in.
__attribute__((section(".text._start"), noreturn, used)) void _start() {
    s650_test_buffer[0] = 0x53363530u;
    s650_test_buffer[1] = 0x48454C4Cu;
    s650_test_buffer[2] = 1u;
    s650_test_buffer[3] = 2u;
    s650_test_buffer[4] = 3u;
    s650_test_buffer[5] = 4u;
    s650_test_buffer[6] = 5u;
    s650_test_buffer[7] = 0xFFFFFFFFu;
    for (;;) {
    }
}

}  // extern "C"
