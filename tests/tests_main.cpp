#include <cstring>
#include "TestFramework.h"
int main(int argc, char** argv) {
    int ran = 0, failedCases = 0;
    for (auto& c : testfw::cases()) {
        if (argc > 1 && !std::strstr(c.name, argv[1])) continue;
        int before = testfw::failures();
        c.fn();
        ++ran;
        bool ok = testfw::failures() == before;
        if (!ok) ++failedCases;
        std::printf("[%s] %s\n", ok ? " ok " : "FAIL", c.name);
    }
    std::printf("\n%d tests, %d checks, %d failed checks, %d failed tests\n", ran, testfw::checks(), testfw::failures(), failedCases);
    return testfw::failures() ? 1 : 0;
}
