#include "core/Clock.h"
#include <chrono>
#include <ctime>

namespace s650 {

static uint64_t steadyMs() {
    using namespace std::chrono;
    return uint64_t(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

SystemClock::SystemClock() {
    std::time_t t = std::time(nullptr);
    std::tm lt{};
    localtime_r(&t, &lt);
    start_ = int64_t(t) + lt.tm_gmtoff - int64_t(steadyMs() / 1000);
}
uint64_t SystemClock::nowMs() const { return steadyMs(); }
int64_t SystemClock::wallSeconds() const { return start_ + int64_t(steadyMs() / 1000); }

}  // namespace s650
