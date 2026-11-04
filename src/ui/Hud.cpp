#include "core/Translation.h"
#include "ui/Hud.h"
#include "core/OptionsText.h"
#include <algorithm>
#include <cstdio>
#include "core/Clock.h"
#include "core/DateTime.h"
#include "core/Strings.h"
#include "display/Icons.h"
#include "ui/Widgets.h"

namespace s650 {
namespace {
std::string translatedKey(const char* group, const char* suffix) {
    FixedKey<64> key(group, ".", suffix);
    return key.valid() ? tr(key.c_str()) : std::string();
}
const Color kScrim(6, 7, 8);
constexpr int kScrimA = 117;
constexpr int kTopH = 20, kBottomH = 21;

TextStyle over(Color c, bool bold = false, int scale = 1, int alpha = 255) { TextStyle s = style(c, bold, scale, alpha); s.shadow = true; return s; }

void bracket(Canvas& cv, float cx, float cy, float w, float h, float th, Color c, int a) {
    float x1 = cx - w / 2, x2 = cx + w / 2, y1 = cy - h / 2, y2 = cy + h / 2, k = std::min(8.f, w / 3);
    cv.line(x1, y1 + k, x1, y1, th, c, a); cv.line(x1, y1, x1 + k, y1, th, c, a);
    cv.line(x2 - k, y1, x2, y1, th, c, a); cv.line(x2, y1, x2, y1 + k, th, c, a);
    cv.line(x2, y2 - k, x2, y2, th, c, a); cv.line(x2, y2, x2 - k, y2, th, c, a);
    cv.line(x1 + k, y2, x1, y2, th, c, a); cv.line(x1, y2, x1, y2 - k, th, c, a);
}

void drawGrid(Canvas& cv, int kind) {
    const float x0 = 0, y0 = kTopH, w = kLcdW, h = kLcdH - kTopH - kBottomH;
    auto L = [&](float ax, float ay, float bx, float by) { cv.line(x0 + ax * w / 100, y0 + ay * h / 100, x0 + bx * w / 100, y0 + by * h / 100, 1.f, Color(255, 255, 255), 96); };
    if (kind == 1) { L(33.3f, 0, 33.3f, 100); L(66.7f, 0, 66.7f, 100); L(0, 33.3f, 100, 33.3f); L(0, 66.7f, 100, 66.7f); }
    else if (kind == 2) { L(50, 0, 50, 100); L(0, 50, 100, 50); L(25, 25, 75, 25); L(75, 25, 75, 75); L(75, 75, 25, 75); L(25, 75, 25, 25); }
    else if (kind == 3) { L(0, 0, 100, 100); L(100, 0, 0, 100); L(50, 0, 50, 100); L(0, 50, 100, 50); }
}

const char* kWbShort[6] = {"AWB", "DAY", "CLOUD", "FLUOR", "INCAND", "FLASH"};
}  // namespace

void drawHud(Canvas& cv, const HudModel& m) {
    const Settings& S = *m.settings;
    const auto& d = S.display;
    const auto& it = d.items;
    const Color white(255, 255, 255), dim(200, 205, 210), amber(255, 178, 77);

    if (d.grid != 0) drawGrid(cv, d.grid);

    if (d.showInfo) {
        // ---- top bar ----
        cv.fillRect({0, 0, kLcdW, kTopH}, kScrim, kScrimA);
        int x = 7;
        std::string mode = translatedKey("mode", kModeKeys[S.shoot.mode]);
        int mw = Canvas::textWidth(mode, 1, true);
        cv.strokeRect({x, 3, mw + 8, 14}, white, 1, 217);
        cv.text(x + 4, 6, mode, over(white, true));
        x += mw + 8 + 7;
        if (S.shoot.mode == 2) x += cv.text(x, 6, translatedKey("scene", kSceneKeys[S.shoot.scene]), over(dim)) + 7;
        if (m.recSeconds >= 0) {
            char b[24]; std::snprintf(b, sizeof b, "\xE2\x97\x8F REC %02d:%02d", m.recSeconds / 60, m.recSeconds % 60);
            x += cv.text(x, 6, b, over(Color(255, 107, 94), true)) + 7;
        } else if (it.size) {
            x += cv.text(x, 6, kSizes[S.shoot.size].id, over(white, true)) + 4;
            x += cv.text(x, 6, S.shoot.quality == 0 ? "FINE" : "STD", over(dim)) + 7;
        }
        if (it.flash && m.eff.mode != 3) {
            drawIcon(cv, m.eff.flash == 3 ? Icon::FlashOff : Icon::Flash, x, 3, 13, white);
            x += 15;
            const char* suffix = m.eff.flash == 0 ? "A" : m.eff.flash == 2 ? "SL" : "";
            cv.text(x, 6, suffix, over(dim));
        }
        int rx = kLcdW - 7;
        if (it.battery && m.batteryPercent >= 0) {
            bool blink = m.batteryPercent <= 12 && (m.timeMs / 500) % 2;
            if (!blink) drawBattery(cv, rx - 23, 4, 23, 12, m.batteryPercent, m.batteryPercent <= 12 ? Color(255, 122, 107) : white);
            rx -= 29;
        }
        if (it.remaining) {
            drawIcon(cv, Icon::Card, rx - 13, 3, 13, white); rx -= 18;
            cv.textRight(rx, 6, std::to_string(m.remaining), over(white, true));
        }

        // ---- exposure chips and readout ----
        if (it.exposure != 0) {
            bool always = it.exposure == 2;
            bool evOn = m.eff.ev != 6, wbOn = m.eff.wb != 0, isoOn = m.eff.iso != 0;
            int cx = 7, cy = 203;
            if (always || evOn) cx += cv.text(cx, cy, "EV " + evLabel(m.eff.ev), over(evOn ? amber : dim, evOn)) + 9;
            if (always || wbOn) cx += cv.text(cx, cy, kWbShort[m.eff.wb], over(wbOn ? amber : dim, wbOn)) + 9;
            if (always || isoOn) {
                std::string v = isoOn ? std::to_string(kIsoValues[m.eff.iso]) : "AUTO";
                cv.text(cx, cy, "ISO " + v, over(isoOn ? amber : dim, isoOn));
            }
        }
        if (it.readout) {
            std::string a = m.exposure.valid ? m.exposure.shutter : "--", b = m.exposure.valid ? m.exposure.aperture : "--";
            int total = Canvas::textWidth(a, 2) + 12 + Canvas::textWidth(b, 2);
            int sx = (kLcdW - total) / 2;
            sx += cv.text(sx, 25, a, over(white, false, 2)) + 12;
            cv.text(sx, 25, b, over(white, false, 2));
        }

        // ---- bottom bar ----
        cv.fillRect({0, kLcdH - kBottomH, kLcdW, kBottomH}, kScrim, kScrimA);
        int by = kLcdH - kBottomH + 7;
        if (it.datetime) cv.text(7, by, formatDateTime(m.wall), over(white));
        if (it.zoom) {
            int zx = 118;
            zx += cv.text(zx, by, "W", over(dim)) + 5;
            cv.strokeRect({zx, by + 2, 70, 4}, white, 1);
            cv.fillRect({zx + 1, by + 3, std::min(68, 68 * S.shoot.zoom / 25), 2}, white);
            zx += 75;
            zx += cv.text(zx, by, "T", over(dim)) + 5;
            int mm = int(35.f + float(m.eff.zoom) / 25.f * 70.f);
            cv.text(zx, by, std::to_string(mm) + "mm", over(dim));
        }
        int rx2 = kLcdW - 7;
        if (S.shoot.macro) { drawIcon(cv, Icon::Macro, rx2 - 13, by - 3, 13, white); rx2 -= 18; }
        if (S.shoot.timer != 0) {
            std::string t = translatedKey("timer", kTimerKeys[S.shoot.timer]);
            rx2 -= cv.textRight(rx2, by, t, over(white)) + 4;
            drawIcon(cv, Icon::Timer, rx2 - 13, by - 3, 13, white);
        }
    }

    if (d.histogram) {
        Rect r{kLcdW - 7 - 74, 25, 74, 32};
        cv.fillRect(r, kScrim, 128);
        cv.strokeRect(r, white, 1, 128);
        for (size_t i = 0; i < m.hist.size() && i < 32; ++i) {
            int h = std::max(1, int(m.hist[i] * 28.f));
            cv.fillRect({r.x + 1 + int(i) * 2, r.y + r.h - 2 - h, 1, h}, white, 224);
        }
    }

    if (m.preview) return;

    // ---- autofocus frames ----
    bool multi = S.shoot.af == 0 && m.eff.mode != 3;
    bool blinkOn = (m.timeMs / 175) % 2 == 0;
    if (m.focus == FocusState::Locked) {
        Color g(88, 214, 141);
        if (multi) { bracket(cv, 160, 120, 54, 40, 1.8f, g, 255); bracket(cv, 104, 124, 34, 26, 1.8f, g, 255); bracket(cv, 216, 124, 34, 26, 1.8f, g, 255); }
        else bracket(cv, 160, 120, 44, 34, 1.8f, g, 255);
    } else if (m.focus == FocusState::Idle || blinkOn) {
        if (multi) bracket(cv, 160, 120, 54, 40, 1.4f, white, 235); else bracket(cv, 160, 120, 44, 34, 1.4f, white, 235);
    }

    if (m.countdown >= 0) {
        std::string n = std::to_string(m.countdown);
        cv.textCenter(160, 68, n, over(white, true, 5));
    }
    if (!m.toast.empty()) {
        int w = Canvas::textWidth(m.toast) + 20;
        cv.fillRect({(kLcdW - w) / 2, 26, w, 17}, kScrim, 209);
        cv.textCenter(kLcdW / 2, 31, m.toast, style(white));
    }
    if (m.simulatedBackend) cv.text(kLcdW - 7 - Canvas::textWidth("SIM"), kTopH + 4 + (d.histogram ? 36 : 0), "SIM", over(Color(255, 178, 77), true));
}

}  // namespace s650
