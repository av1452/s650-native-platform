#pragma once
// Shooting-screen overlay (port of js/hud.js + css/hud.css). Pure drawing: it reads values, it never talks to hardware.
#include <string>
#include <vector>
#include "camera/CameraBackend.h"
#include "core/Settings.h"
#include "display/Framebuffer.h"

namespace s650 {

enum class FocusState { Idle, Seeking, Locked };

struct HudModel {
    const Settings* settings = nullptr;
    Effective eff{};
    ExposureInfo exposure;             // exposure.valid == false renders "--" (camera unavailable)
    int remaining = 0, batteryPercent = -1;
    int64_t wall = 0;
    FocusState focus = FocusState::Idle;
    int countdown = -1;                // seconds left, -1 = none
    int recSeconds = -1;               // -1 = not recording
    std::string toast;
    std::vector<float> hist;
    uint64_t timeMs = 0;               // drives blinking
    bool preview = false;              // display-options preview: no AF frames, toast or countdown
    bool simulatedBackend = false;     // shows a small "SIM" tag so a simulation is never mistaken for a camera
};

void drawHud(Canvas& cv, const HudModel& m);
}  // namespace s650
