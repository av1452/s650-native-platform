#include "core/SystemController.h"

namespace s650 {

void SystemController::pressPower() {
    switch (state_) {
        case PowerState::Off:
            lastActivity_ = clock_.nowMs();
            animated_ = settings_.setup.startup;
            if (animated_) enter(PowerState::Booting); else { enter(PowerState::On); pendingBootFinished_ = true; }
            break;
        case PowerState::Booting: skipBoot(); break;
        case PowerState::On: enter(PowerState::ShuttingDown); break;
        case PowerState::ShuttingDown: break;
    }
}

void SystemController::skipBoot() {
    if (state_ != PowerState::Booting) return;
    enter(PowerState::On);
    lastActivity_ = clock_.nowMs();
    pendingBootFinished_ = true;
}

PowerEvent SystemController::update() {
    if (pendingBootFinished_) { pendingBootFinished_ = false; return PowerEvent::BootFinished; }
    uint64_t now = clock_.nowMs();
    switch (state_) {
        case PowerState::Booting:
            if (now - since_ >= kBootMs) { enter(PowerState::On); lastActivity_ = now; return PowerEvent::BootFinished; }
            break;
        case PowerState::ShuttingDown:
            if (now - since_ >= kShutdownMs) { enter(PowerState::Off); return PowerEvent::ShutdownFinished; }
            break;
        case PowerState::On: {
            int minutes = kAutoOffMinutes[settings_.setup.autoOff];
            if (minutes > 0 && now - lastActivity_ >= uint64_t(minutes) * 60000ull) enter(PowerState::ShuttingDown);
            break; }
        case PowerState::Off: break;
    }
    return PowerEvent::None;
}

}  // namespace s650
