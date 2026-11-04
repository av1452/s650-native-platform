#pragma once
// Owns the screens and the modal dialog; routes logical input; knows nothing about real hardware except through the
// four backend interfaces it is given.
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "camera/CameraBackend.h"
#include "core/Clock.h"
#include "core/Settings.h"
#include "core/Strings.h"
#include "storage/StorageBackend.h"
#include "ui/Widgets.h"

namespace s650 {

enum class ScreenId { Off, Shooting, Menu, Display, Playback };

class Screen {
public:
    virtual ~Screen() = default;
    virtual void enter(int /*param*/) {}
    virtual void exit() {}
    virtual void onInput(const InputEvent& e) = 0;
    virtual void update(uint32_t /*dtMs*/) {}
    virtual void render(Canvas& cv) = 0;
};

struct Dialog {
    std::string title, message, ok, cancel;
    bool danger = false, focusOk = false;
    std::function<void()> onOk, onCancel;
};

class UIController {
public:
    UIController(Settings& s, StorageBackend& st, CameraBackend& cam, Clock& clk);
    ~UIController();

    Settings& settings;
    StorageBackend& storage;
    CameraBackend& camera;
    Clock& clock;

    void show(ScreenId id, int param = 0);
    ScreenId current() const { return current_; }
    Screen& screen(ScreenId id);

    void handle(const InputEvent& e);        // dialog first, then the current screen
    void update(uint32_t dtMs);
    void render(Canvas& cv);

    void confirm(const Dialog& d);
    bool dialogOpen() const { return dialogOpen_; }
    void closeDialog() { dialogOpen_ = false; }
    const Dialog& dialog() const { return dialog_; }

    void settingsChanged();                  // persists through the StorageBackend; failures are announced, not hidden
    int64_t wallNow() const { return clock.wallSeconds() + settings.setup.clockOffset; }
    int remainingShots() const;

    // Stand-in for the browser prototype's screen-reader live region; also lets tests observe behaviour.
    void announce(const std::string& text) { last_ = text; ++announceCount_; }
    const std::string& lastAnnouncement() const { return last_; }
    int announceCount() const { return announceCount_; }

private:
    std::unique_ptr<Screen> screens_[5];
    ScreenId current_ = ScreenId::Off;
    Dialog dialog_;
    bool dialogOpen_ = false;
    std::string last_;
    int announceCount_ = 0;
};

}  // namespace s650
