#pragma once
// Source of logical button events. Pointer or touch input is deliberately NOT part of the interface.
#include <deque>
#include <string>
#include "core/Types.h"

namespace s650 {
class InputBackend {
public:
    virtual ~InputBackend() = default;
    virtual bool initialize() { return true; }
    virtual bool poll(InputEvent& out) = 0;      // false when no event is pending
    virtual std::string name() const = 0;
};

// Deterministic input for tests and the screenshot tour: events are queued by code.
class ScriptedInput : public InputBackend {
public:
    bool poll(InputEvent& out) override { if (q_.empty()) return false; out = q_.front(); q_.pop_front(); return true; }
    std::string name() const override { return "scripted"; }
    void push(Action a, Phase p = Phase::Press) { q_.push_back({a, p}); }
    void tap(Action a, int times = 1) { for (int i = 0; i < times; ++i) { push(a, Phase::Press); if (a == Action::Shutter) push(a, Phase::Release); } }
    bool empty() const { return q_.empty(); }
private:
    std::deque<InputEvent> q_;
};

// Desktop keyboard mapping (documented in README): arrows, Enter, Esc, M, P, D (DISP), W/T, Space, [ ], O, Delete, K.
Action actionForKey(int keyCode, char ch);
}  // namespace s650
