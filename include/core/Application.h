#pragma once
// Wires the backends, the power state machine and the UI together. One tick = poll input, advance time, draw, present.
#include <memory>
#include <string>
#include "camera/CameraBackend.h"
#include "core/Clock.h"
#include "core/Settings.h"
#include "core/SystemController.h"
#include "display/DisplayBackend.h"
#include "input/InputBackend.h"
#include "storage/StorageBackend.h"
#include "ui/UIController.h"

namespace s650 {

struct Backends {
    DisplayBackend& display;
    InputBackend& input;
    CameraBackend& camera;
    StorageBackend& storage;
    Clock& clock;
};

class Application {
public:
    explicit Application(Backends b);
    // Initialises storage (loading saved settings), camera, display. A camera that fails to initialise is NOT fatal:
    // the UI keeps running and reports it. Returns false only if display or storage cannot start.
    bool start(bool powerOn);
    void tick();                                   // one frame
    void requestQuit() { quit_ = true; }
    bool quitRequested() const { return quit_; }

    Settings& settings() { return settings_; }
    UIController& ui() { return ui_; }
    SystemController& system() { return system_; }
    const Framebuffer& frame() const { return frame_; }
    const std::string& startupLog() const { return log_; }

private:
    void dispatch(const InputEvent& e);
    void render();
    Backends b_;
    Settings settings_;
    SystemController system_;
    UIController ui_;
    Framebuffer frame_;
    uint64_t lastMs_ = 0;
    int bootFadeMs_ = -1;
    bool quit_ = false;
    std::string log_;
};

}  // namespace s650
