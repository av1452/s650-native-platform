#include "core/Application.h"
#include <algorithm>
#include "ui/Boot.h"

namespace s650 {

Application::Application(Backends b)
    : b_(b), system_(b.clock, settings_), ui_(settings_, b.storage, b.camera, b.clock) {}

bool Application::start(bool powerOn) {
    if (!b_.storage.initialize()) { log_ += "storage: initialisation FAILED\n"; return false; }
    Settings loaded;
    if (b_.storage.loadSettings(loaded)) { settings_ = loaded; log_ += "settings: loaded\n"; } else log_ += "settings: defaults (none saved)\n";
    if (!b_.camera.initialize()) log_ += "camera: " + b_.camera.status().description + " (continuing without camera functions)\n";
    else log_ += "camera: " + b_.camera.status().description + "\n";
    if (!b_.display.initialize()) { log_ += "display: initialisation FAILED\n"; return false; }
    log_ += "display: " + b_.display.name() + ", input: " + b_.input.name() + "\n";
    b_.input.initialize();
    lastMs_ = b_.clock.nowMs();
    ui_.show(ScreenId::Off);
    if (powerOn) system_.pressPower();
    render();
    return true;
}

void Application::dispatch(const InputEvent& e) {
    if (e.action == Action::None) return;
    if (e.action == Action::Power) { if (e.phase == Phase::Press) { if (system_.state() == PowerState::On) ui_.closeDialog(); system_.pressPower(); } return; }
    switch (system_.state()) {
        case PowerState::Off: if (e.action == Action::Ok && e.phase == Phase::Press) system_.pressPower(); return;
        case PowerState::Booting: if (e.phase == Phase::Press) system_.skipBoot(); return;
        case PowerState::ShuttingDown: return;
        case PowerState::On: system_.userActivity(); ui_.handle(e); return;
    }
}

void Application::tick() {
    uint64_t now = b_.clock.nowMs();
    uint32_t dt = uint32_t(std::min<uint64_t>(now - lastMs_, 100));
    lastMs_ = now;
    InputEvent e;
    while (b_.input.poll(e)) dispatch(e);
    switch (system_.update()) {
        case PowerEvent::BootFinished:
            ui_.show(ScreenId::Shooting, system_.bootAnimated() ? 1 : 0);
            bootFadeMs_ = system_.bootAnimated() ? 0 : -1;
            break;
        case PowerEvent::ShutdownFinished: ui_.show(ScreenId::Off); break;
        case PowerEvent::None: break;
    }
    if (system_.state() == PowerState::On || system_.state() == PowerState::ShuttingDown) ui_.update(dt);
    if (bootFadeMs_ >= 0) { bootFadeMs_ += int(dt); if (bootFadeMs_ > 340) bootFadeMs_ = -1; }
    render();
    Framebuffer out = frame_;
    out.scaleLuma(0.64f + 0.12f * float(settings_.setup.brightness));    // stand-in for the backlight level
    b_.display.present(out);
}

void Application::render() {
    Canvas cv(frame_);
    switch (system_.state()) {
        case PowerState::Off: renderOff(cv); break;
        case PowerState::Booting: renderBoot(cv, system_.elapsedMs()); break;
        case PowerState::On:
            ui_.render(cv);
            if (bootFadeMs_ >= 0) {                       // boot screen dissolves over the live view
                Framebuffer logo; Canvas lc(logo); renderBoot(lc, SystemController::kBootMs);
                cv.blit(logo, {0, 0, kLcdW, kLcdH}, {0, 0, kLcdW, kLcdH}, 255 - std::min(255, bootFadeMs_ * 255 / 340));
            }
            break;
        case PowerState::ShuttingDown: {
            uint32_t t = system_.elapsedMs();
            if (t < 280) { ui_.render(cv); cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0), std::min(255, int(t) * 255 / 260)); }
            else renderShutdownLogo(cv, t);
            break; }
    }
}

}  // namespace s650
