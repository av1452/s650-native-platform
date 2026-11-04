#include "core/Settings.h"
#include <algorithm>

namespace s650 {

Effective effective(const Settings& s) {
    const auto& c = s.shoot;
    bool autoMode = c.mode == 0;
    Effective e{};
    e.mode = c.mode; e.scene = c.scene;
    e.ev = autoMode ? 6 : c.ev; e.wb = autoMode ? 0 : c.wb; e.iso = autoMode ? 0 : c.iso;
    e.flash = c.mode == 3 ? 3 : c.flash;
    e.zoom = c.zoom; e.af = c.af; e.timer = c.timer; e.macro = c.macro;
    return e;
}
bool isLockedInAuto(const Settings& s) { return s.shoot.mode == 0; }

DisplayItems presetItems(int d) {
    DisplayItems i;
    if (d == 0) { i.size = false; i.flash = false; i.datetime = false; i.exposure = 0; }
    else if (d == 2) { i.exposure = 2; i.readout = true; }
    return i;   // standard
}
void applyDensity(Settings& s, int d) { s.display.items = presetItems(d); s.display.density = d; }

void initializeSettings(Settings& s) {
    int clock = s.setup.clockOffset;
    Settings fresh;
    s = fresh;
    s.setup.clockOffset = clock;
}

int estimatedBytes(const Settings& s) {
    const SizeOption& z = kSizes[std::clamp(s.shoot.size, 0, 5)];
    return s.shoot.quality == 0 ? z.fineBytes : z.stdBytes;
}

}  // namespace s650
