#include "ui/UIController.h"
#include <algorithm>
#include "ui/Screens.h"

namespace s650 {

UIController::UIController(Settings& s, StorageBackend& st, CameraBackend& cam, Clock& clk)
    : settings(s), storage(st), camera(cam), clock(clk) {
    screens_[int(ScreenId::Shooting)] = std::make_unique<ShootingScreen>(*this);
    screens_[int(ScreenId::Menu)] = std::make_unique<MenuScreen>(*this);
    screens_[int(ScreenId::Display)] = std::make_unique<DisplayOptionsScreen>(*this);
    screens_[int(ScreenId::Playback)] = std::make_unique<PlaybackScreen>(*this);
}
UIController::~UIController() = default;

Screen& UIController::screen(ScreenId id) { return *screens_[int(id)]; }

void UIController::show(ScreenId id, int param) {
    if (screens_[int(current_)]) screens_[int(current_)]->exit();
    dialogOpen_ = false;
    current_ = id;
    if (screens_[int(id)]) screens_[int(id)]->enter(param);
}

void UIController::confirm(const Dialog& d) {
    dialog_ = d; dialogOpen_ = true;
    announce(d.title + ". " + d.message + " " + (d.focusOk ? d.ok : d.cancel));
}

void UIController::handle(const InputEvent& e) {
    if (dialogOpen_) {
        if (e.phase != Phase::Press) return;
        Dialog d = dialog_;                    // copy: callbacks may open another dialog
        switch (e.action) {
            case Action::Left: case Action::Right: case Action::Up: case Action::Down:
                dialog_.focusOk = !dialog_.focusOk;
                announce(dialog_.focusOk ? dialog_.ok : dialog_.cancel); break;
            case Action::Ok: dialogOpen_ = false; if (d.focusOk) { if (d.onOk) d.onOk(); } else if (d.onCancel) d.onCancel(); break;
            case Action::Back: case Action::Menu: dialogOpen_ = false; if (d.onCancel) d.onCancel(); break;
            default: break;
        }
        return;
    }
    if (screens_[int(current_)]) screens_[int(current_)]->onInput(e);
}

void UIController::update(uint32_t dt) { if (screens_[int(current_)]) screens_[int(current_)]->update(dt); }

void UIController::render(Canvas& cv) {
    if (!screens_[int(current_)]) { cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0)); return; }
    screens_[int(current_)]->render(cv);
    if (dialogOpen_) {
        DialogView v; v.title = dialog_.title; v.message = dialog_.message; v.ok = dialog_.ok; v.cancel = dialog_.cancel; v.danger = dialog_.danger; v.focusOk = dialog_.focusOk;
        drawDialog(cv, palette(settings.display.style), v);
    }
}

void UIController::settingsChanged() {
    Result r = storage.saveSettings(settings);
    if (!r.ok) announce("Settings not saved: " + r.error);
}

int UIController::remainingShots() const {
    int per = estimatedBytes(settings);
    return per > 0 ? int(storage.freeBytes() / uint64_t(per)) : 0;
}

}  // namespace s650
