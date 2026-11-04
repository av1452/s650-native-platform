#pragma once
// A complete HEADLESS, fully SIMULATED camera: manual clock, scripted buttons, in-memory display/storage,
// simulated camera. Used by the screenshot tour and the tests. Nothing here touches hardware.
#include <string>
#include "camera/CameraBackend.h"
#include "core/Application.h"
#include "core/Clock.h"
#include "display/DisplayBackend.h"
#include "display/Png.h"
#include "input/InputBackend.h"
#include "storage/StorageBackend.h"

namespace s650 {

struct DesktopRig {
    ManualClock clock;
    HeadlessDisplay display;
    ScriptedInput input;
    SimulatedCamera camera;
    SimulatedStorage storage;
    Application app;

    explicit DesktopRig(const std::string& settingsPath = "", bool seedSamples = true)
        : storage(settingsPath, seedSamples), app(Backends{display, input, camera, storage, clock}) {}

    // Advance simulated time: about 60 frames per second, or 10 per second for long waits (the application caps one
    // frame's time step at 100 ms, so this still advances every timer correctly and keeps long tests fast).
    void advance(uint32_t ms) {
        const uint32_t frame = ms > 3000 ? 100 : 16;
        while (ms > 0) { uint32_t step = ms < frame ? ms : frame; clock.advance(step); app.tick(); ms -= step; }
    }
    void tap(Action a, int times = 1) { for (int i = 0; i < times; ++i) { input.tap(a); advance(32); } }
    // Start powered on and skip the startup animation so the first screen is the live view.
    bool startReady() {
        if (!app.start(false)) return false;
        app.settings().setup.startup = false;
        input.tap(Action::Power);
        advance(64);
        return true;
    }
    bool savePng(const std::string& path, int scale = 2) { return display.savePng(path, scale); }
};

}  // namespace s650
