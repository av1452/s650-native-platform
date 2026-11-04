#include "core/SettingsIO.h"
#include <algorithm>
#include <functional>
#include <sstream>
#include <vector>

namespace s650 {

namespace {
struct Field { const char* key; std::function<int()> get; std::function<void(int)> set; int lo, hi; };

std::vector<Field> fields(Settings& s) {
    auto I = [](int& r, const char* k, int lo, int hi) { return Field{k, [&r] { return r; }, [&r](int v) { r = v; }, lo, hi}; };
    auto B = [](bool& r, const char* k) { return Field{k, [&r] { return r ? 1 : 0; }, [&r](int v) { r = v != 0; }, 0, 1}; };
    auto& c = s.shoot; auto& u = s.setup; auto& d = s.display; auto& it = d.items; auto& p = s.playback;
    return {I(c.mode, "mode", 0, 3), I(c.scene, "scene", 0, 3), I(c.timer, "timer", 0, 2), I(c.size, "size", 0, 5),
            I(c.quality, "quality", 0, 1), I(c.af, "af", 0, 1), I(c.flash, "flash", 0, 3), I(c.wb, "wb", 0, 5),
            I(c.iso, "iso", 0, 5), I(c.ev, "ev", 0, 12), I(c.zoom, "zoom", 0, 25), B(c.macro, "macro"), B(c.redeye, "redeye"),
            I(u.brightness, "brightness", 1, 5), I(u.beep, "beep", 0, 2), I(u.usb, "usb", 0, 2), I(u.autoOff, "autoOff", 0, 3),
            B(u.startup, "startup"), I(u.clockOffset, "clockOffset", -2000000000, 2000000000),
            B(d.showInfo, "showInfo"), I(d.density, "density", 0, 3), B(it.battery, "i.battery"), B(it.remaining, "i.remaining"),
            B(it.size, "i.size"), B(it.flash, "i.flash"), B(it.datetime, "i.datetime"), B(it.zoom, "i.zoom"),
            I(it.exposure, "i.exposure", 0, 2), B(it.readout, "i.readout"), B(d.histogram, "histogram"),
            I(d.grid, "grid", 0, 3), I(d.style, "style", 0, 2), I(p.interval, "interval", 0, 2), B(p.repeat, "repeat")};
}
}  // namespace

std::string serialize(const Settings& in) {
    Settings s = in;
    std::ostringstream o;
    o << "S650SETTINGS 1\n";
    for (auto& f : fields(s)) o << f.key << '=' << f.get() << '\n';
    return o.str();
}

bool deserialize(const std::string& text, Settings& out) {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || line.rfind("S650SETTINGS", 0) != 0) return false;
    Settings s;                       // unknown or missing keys keep their defaults
    auto fs = fields(s);
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        long v = 0;
        try { v = std::stol(line.substr(eq + 1)); } catch (...) { continue; }
        for (auto& f : fs) if (k == f.key) { f.set(int(std::clamp<long>(v, f.lo, f.hi))); break; }
    }
    out = s;
    return true;
}

}  // namespace s650
