#pragma once
// Power state machine: Off -> Booting -> On -> ShuttingDown -> Off, plus auto power-off. This is the SIMULATED
// application lifecycle; it does not (and cannot yet) control the real camera's power or boot.
#include <cstdint>
#include "core/Clock.h"
#include "core/Settings.h"

namespace s650 {

enum class PowerState { Off, Booting, On, ShuttingDown };
enum class PowerEvent { None, BootFinished, ShutdownFinished };

class SystemController {
public:
    static constexpr uint32_t kBootMs = 1900, kShutdownMs = 1350;
    SystemController(const Clock& clk, const Settings& s) : clock_(clk), settings_(s) {}

    PowerState state() const { return state_; }
    uint32_t elapsedMs() const { return uint32_t(clock_.nowMs() - since_); }
    bool bootAnimated() const { return animated_; }

    void pressPower();               // toggles: Off -> boot, On -> shutdown (ignored while a transition runs, except boot skip)
    void skipBoot();
    void userActivity() { lastActivity_ = clock_.nowMs(); }
    PowerEvent update();             // call once per frame; may start auto power-off

private:
    void enter(PowerState s) { state_ = s; since_ = clock_.nowMs(); }
    const Clock& clock_;
    const Settings& settings_;
    PowerState state_ = PowerState::Off;
    uint64_t since_ = 0, lastActivity_ = 0;
    bool animated_ = true, pendingBootFinished_ = false;
};

}  // namespace s650
