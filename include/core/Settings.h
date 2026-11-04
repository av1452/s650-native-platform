#pragma once
// All user-changeable state. Plain data, no behaviour that touches hardware. Persisted by a StorageBackend.
#include <stdint.h>
#include "core/Options.h"

namespace s650 {

struct DisplayItems {
    bool battery = true, remaining = true, size = true, flash = true, datetime = true, zoom = true;
    int exposure = 1;          // 0 off, 1 when changed, 2 always
    bool readout = false;      // shutter / aperture
};

struct Settings {
    struct Shooting {
        int mode = 0, scene = 0, timer = 0, size = 0, quality = 0, af = 0, flash = 0, wb = 0, iso = 0;
        int ev = 6;            // index into the 13 EV steps; 6 = 0.0
        int zoom = 0;          // 0..25
        bool macro = false, redeye = true;
    } shoot;
    struct Setup {
        int brightness = 3;    // 1..5
        int beep = 1, usb = 0, autoOff = 3;   // autoOff index into {0,1,2,5} minutes
        bool startup = true;
        int clockOffset = 0;   // seconds added to the clock's wall time
    } setup;
    struct Display {
        bool showInfo = true;
        int density = 1;       // 0 minimal 1 standard 2 detailed 3 custom
        DisplayItems items;
        bool histogram = false;
        int grid = 0, style = 0;
    } display;
    struct Playback { int interval = 1; bool repeat = false; } playback;
};

// What the camera would actually apply: Auto mode locks EV/WB/ISO (ASSUMPTION from a retailer's wording),
// and Movie mode turns the flash off.
struct Effective { int mode, scene, ev, wb, iso, flash, zoom, af, timer; bool macro; };
Effective effective(const Settings& s);
bool isLockedInAuto(const Settings& s);

DisplayItems presetItems(int density);          // minimal / standard / detailed
void applyDensity(Settings& s, int density);    // sets items from a preset
void initializeSettings(Settings& s);           // "Initialize": defaults, but clock offset is kept

// Size and shots-left helpers
int estimatedBytes(const Settings& s);

}  // namespace s650
