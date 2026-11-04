#pragma once
// Time source abstraction so that animation, timers and auto power-off can be tested deterministically.
#include <stdint.h>

namespace s650 {

class Clock {
public:
    virtual ~Clock() = default;
    virtual uint64_t nowMs() const = 0;          // monotonic milliseconds
    virtual int64_t wallSeconds() const = 0;     // "local wall time" as seconds since 1970 (naive, no time zones)
};

class SystemClock : public Clock {
public:
    SystemClock();
    uint64_t nowMs() const override;
    int64_t wallSeconds() const override;
private:
    int64_t start_;
};

// Test clock: time only moves when told to.
class ManualClock : public Clock {
public:
    explicit ManualClock(int64_t wallStart = 1177754400 /* 2007-04-28 10:00:00 */) : wall_(wallStart) {}
    uint64_t nowMs() const override { return ms_; }
    int64_t wallSeconds() const override { return wall_ + int64_t(ms_ / 1000); }
    void advance(uint64_t ms) { ms_ += ms; }
private:
    uint64_t ms_ = 0;
    int64_t wall_;
};

}  // namespace s650
