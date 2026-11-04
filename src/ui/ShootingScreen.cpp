#include "core/Translation.h"
#include <algorithm>
#include "camera/Exposure.h"
#include "ui/Screens.h"

namespace s650 {
namespace {
std::string translatedKey(const char* group, const char* suffix) {
    FixedKey<64> key(group, ".", suffix);
    return key.valid() ? tr(key.c_str()) : std::string();
}
}  // namespace

void ShootingScreen::enter(int param) {
    halfDown_ = false; focus_ = FocusState::Idle; countdownMs_ = -1; toast_.clear(); toastMs_ = 0;
    pendingShot_ = false; preflashStep_ = -1; flashMs_ = 0; haveLive_ = false;
    settleMs_ = param == 1 ? 0 : -1;
    recording_ = false;
}

void ShootingScreen::exit() {
    if (recording_) { ui_.camera.setMovieRecording(false); recording_ = false; }
}

void ShootingScreen::toast(const std::string& t) { toast_ = t; toastMs_ = 1400; ui_.announce(t); }

void ShootingScreen::cancelCountdown() {
    if (countdownMs_ < 0) return;
    countdownMs_ = -1; toast(tr("toast.cancel"));
}

void ShootingScreen::cycleDisplay() {
    auto& d = ui_.settings.display;
    std::string name;
    if (!d.showInfo) { d.showInfo = true; applyDensity(ui_.settings, 1); name = "density.standard"; }
    else if (d.density == 1) { applyDensity(ui_.settings, 2); name = "density.detailed"; }
    else if (d.density == 2) { applyDensity(ui_.settings, 0); name = "density.minimal"; }
    else if (d.density == 0) { d.showInfo = false; name = "opt.hide"; }
    else { applyDensity(ui_.settings, 1); name = "density.standard"; }
    ui_.settingsChanged();
    toast(tr("toast.display") + ": " + tr(name));
}

void ShootingScreen::capture() {
    Effective eff = effective(ui_.settings);
    if (ui_.remainingShots() <= 0) { toast(tr("toast.full")); return; }
    CaptureResult r = ui_.camera.capture(eff, ui_.wallNow());
    if (!r.status.ok) { toast(r.status.error); return; }     // fails visibly, never silently
    ImageRecord rec;
    const SizeOption& z = kSizes[ui_.settings.shoot.size];
    rec.w = z.w; rec.h = z.h; rec.fine = ui_.settings.shoot.quality == 0;
    rec.bytes = uint64_t(double(rec.fine ? z.fineBytes : z.stdBytes) * (0.92 + 0.16 * double((time_ / 7) % 100) / 100.0));
    rec.taken = ui_.wallNow(); rec.meta = r.meta; rec.pixels = r.image;
    Result add = ui_.storage.addImage(rec);
    if (!add.ok) { toast(add.error); return; }
    flashPeak_ = r.flashFired ? 235 : 56;
    flashMs_ = r.flashFired ? 140 : 90;
    toast(tr("toast.saved") + "  " + ui_.storage.images().back().name);
}

// Red-eye reduction: two short pre-flashes before the (simulated) exposure when the flash will fire.
void ShootingScreen::beginShot() {
    Effective eff = effective(ui_.settings);
    if (ui_.settings.shoot.redeye && ui_.camera.flashWouldFire(eff)) { preflashStep_ = 0; preflashMs_ = 0; pendingShot_ = true; }
    else capture();
}

void ShootingScreen::onInput(const InputEvent& e) {
    auto& st = ui_.settings.shoot;
    bool press = e.phase == Phase::Press;
    if (e.action == Action::Shutter) {
        if (press) {
            if (countdownMs_ >= 0) { cancelCountdown(); halfDown_ = false; return; }
            if (recording_) { halfDown_ = true; return; }
            halfDown_ = true; afMs_ = 0;
            // autofocus is a camera function: with no working camera backend nothing may claim to focus
            focus_ = ui_.camera.status().available ? FocusState::Seeking : FocusState::Idle;
        } else if (halfDown_) {
            halfDown_ = false;
            if (st.mode == 3) {
                if (!recording_) {
                    Result r = ui_.camera.setMovieRecording(true);
                    if (!r.ok) toast(r.error); else { recording_ = true; recStart_ = time_; }
                } else { ui_.camera.setMovieRecording(false); recording_ = false; toast(tr("toast.recstop")); }
            } else if (st.timer != 0) countdownMs_ = st.timer == 1 ? 2000 : 10000;
            else beginShot();
            if (focus_ != FocusState::Idle) afMs_ = -350;     // hold the frame briefly after release
        }
        return;
    }
    if (!press) return;
    switch (e.action) {
        case Action::Up: case Action::Disp: cycleDisplay(); break;
        case Action::Down: st.timer = (st.timer + 1) % 3; ui_.settingsChanged(); toast(tr("toast.timer") + ": " + translatedKey("timer", kTimerKeys[st.timer])); break;
        case Action::Right:
            if (st.mode == 3) break;
            st.flash = (st.flash + 1) % 4; ui_.settingsChanged();
            toast(tr("toast.flash") + ": " + translatedKey("flash", kFlashKeys[st.flash])); break;
        case Action::Left: st.macro = !st.macro; ui_.settingsChanged(); toast(tr("toast.macro") + ": " + tr(st.macro ? "opt.on" : "opt.off")); break;
        case Action::Tele: st.zoom = std::min(25, st.zoom + 1); ui_.settingsChanged(); break;
        case Action::Wide: st.zoom = std::max(0, st.zoom - 1); ui_.settingsChanged(); break;
        case Action::ModeNext: case Action::ModePrev:
            if (recording_) { ui_.camera.setMovieRecording(false); recording_ = false; }
            st.mode = (st.mode + (e.action == Action::ModeNext ? 1 : 3)) % 4; ui_.settingsChanged();
            toast(translatedKey("modename", kModeKeys[st.mode])); break;
        case Action::Menu: ui_.show(ScreenId::Menu, -1); break;
        case Action::Play: ui_.show(ScreenId::Playback, 0); break;
        case Action::Back: cancelCountdown(); break;
        default: break;
    }
}

void ShootingScreen::update(uint32_t dt) {
    time_ += dt;
    if (settleMs_ >= 0) { settleMs_ += int(dt); if (settleMs_ > 520) settleMs_ = -1; }
    if (toastMs_ > 0) { toastMs_ -= int(dt); if (toastMs_ <= 0) toast_.clear(); }
    if (flashMs_ > 0) flashMs_ -= int(dt);
    if (halfDown_ && focus_ == FocusState::Seeking) { afMs_ += int(dt); if (afMs_ >= 240) focus_ = FocusState::Locked; }
    else if (!halfDown_ && focus_ != FocusState::Idle) { afMs_ += int(dt); if (afMs_ >= 0) { focus_ = FocusState::Idle; afMs_ = 0; } }
    if (countdownMs_ >= 0) {
        countdownMs_ -= int(dt);
        if (countdownMs_ <= 0) { countdownMs_ = -1; beginShot(); }
    }
    if (preflashStep_ >= 0) {                       // 0:on 1:off 2:on 3:off then shoot
        preflashMs_ += int(dt);
        if (preflashMs_ >= 90) { preflashMs_ = 0; ++preflashStep_; }
        if (preflashStep_ >= 4) { preflashStep_ = -1; if (pendingShot_) { pendingShot_ = false; capture(); } }
    }
    if (recording_ && ui_.settings.shoot.mode != 3) { ui_.camera.setMovieRecording(false); recording_ = false; }
    histMs_ += int(dt);
}

void ShootingScreen::render(Canvas& cv) {
    Framebuffer& fb = cv.framebuffer();
    Effective eff = effective(ui_.settings);
    haveLive_ = ui_.camera.liveFrame(live_, eff, time_);
    if (haveLive_) {
        float s = settleMs_ >= 0 ? 1.14f - 0.14f * std::min(1.f, float(settleMs_) / 520.f) : 1.f;
        int sw = int(float(kLcdW) / s), sh = int(float(kLcdH) / s);
        if (s == 1.f) cv.blit(live_, {0, 0, kLcdW, kLcdH}, {0, 0, kLcdW, kLcdH});
        else cv.blit(live_, {(kLcdW - sw) / 2, (kLcdH - sh) / 2, sw, sh}, {0, 0, kLcdW, kLcdH});
        if (ui_.settings.display.histogram && (hist_.empty() || histMs_ > 400)) { hist_ = histogram(live_, 32); histMs_ = 0; }
    } else {
        fb.fill(Color(10, 10, 12));
        TextStyle t = style(Color(255, 122, 107), true, 2);
        cv.textCenter(160, 92, "NO CAMERA", t);
        cv.textCenter(160, 118, ui_.camera.status().description, style(Color(160, 165, 170)));
        hist_.assign(32, 0.f);
    }
    HudModel m;
    m.settings = &ui_.settings; m.eff = eff; m.exposure = ui_.camera.exposure(eff);
    m.remaining = ui_.remainingShots(); m.batteryPercent = ui_.camera.status().batteryPercent;
    m.wall = ui_.wallNow(); m.focus = focus_; m.countdown = countdownSeconds();
    m.recSeconds = recording_ ? int((time_ - recStart_) / 1000) : -1;
    m.toast = toast_; m.hist = hist_; m.timeMs = time_; m.simulatedBackend = ui_.camera.status().simulated;
    drawHud(cv, m);
    int a = 0;
    if (flashMs_ > 0) a = flashPeak_;
    if (preflashStep_ >= 0 && preflashStep_ % 2 == 0) a = 140;
    if (a > 0) cv.fillRect({0, 0, kLcdW, kLcdH}, Color(255, 255, 255), a);
}

}  // namespace s650
