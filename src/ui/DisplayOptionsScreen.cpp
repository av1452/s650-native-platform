#include "core/Translation.h"
// Display options (port of js/screens/display.js): 13 items with a live scaled preview of the real overlay.
#include <algorithm>
#include <cmath>
#include "display/Scenes.h"
#include "ui/Screens.h"

namespace s650 {
namespace {
std::string translatedKey(const char* group, const char* suffix) {
    FixedKey<64> key(group, ".", suffix);
    return key.valid() ? tr(key.c_str()) : std::string();
}
struct RowInfo { const char* label; const char* descr; };
const RowInfo kInfo[DisplayOptionsScreen::kRows] = {
    {"disp.showinfo", "dd.showinfo"}, {"disp.preset", "dd.preset"}, {"disp.battery", "dd.battery"}, {"disp.remaining", "dd.remaining"},
    {"disp.size", "dd.size"}, {"disp.flash", "dd.flash"}, {"disp.datetime", "dd.datetime"}, {"disp.zoom", "dd.zoom"},
    {"disp.exposure", "dd.exposure"}, {"disp.readout", "dd.readout"}, {"disp.histogram", "dd.histogram"}, {"disp.grid", "dd.grid"}, {"disp.style", "dd.style"}};

std::string onOff(bool b) { return tr(b ? "opt.on" : "opt.off"); }

std::string valueOf(const Settings& s, int row) {
    const auto& d = s.display; const auto& it = d.items;
    switch (row) {
        case 0: return onOff(d.showInfo);
        case 1: return translatedKey("density", kDensityKeys[d.density]);
        case 2: return onOff(it.battery); case 3: return onOff(it.remaining); case 4: return onOff(it.size); case 5: return onOff(it.flash);
        case 6: return onOff(it.datetime); case 7: return onOff(it.zoom);
        case 8: return translatedKey("expo", kExposureInfoKeys[it.exposure]);
        case 9: return onOff(it.readout); case 10: return onOff(d.histogram);
        case 11: return translatedKey("grid", kGridKeys[d.grid]);
        default: return translatedKey("style", kStyleKeys[d.style]);
    }
}

void change(Settings& s, int row, int dir) {
    auto& d = s.display; auto& it = d.items;
    auto custom = [&] { d.density = 3; };
    switch (row) {
        case 0: d.showInfo = !d.showInfo; break;
        case 1: { int i = d.density > 2 ? 1 : d.density; i = (i + dir + 3) % 3; applyDensity(s, i); break; }
        case 2: it.battery = !it.battery; custom(); break; case 3: it.remaining = !it.remaining; custom(); break;
        case 4: it.size = !it.size; custom(); break; case 5: it.flash = !it.flash; custom(); break;
        case 6: it.datetime = !it.datetime; custom(); break; case 7: it.zoom = !it.zoom; custom(); break;
        case 8: it.exposure = (it.exposure + dir + 3) % 3; custom(); break;
        case 9: it.readout = !it.readout; custom(); break; case 10: d.histogram = !d.histogram; break;
        case 11: d.grid = (d.grid + dir + 4) % 4; break;
        default: d.style = (d.style + dir + 3) % 3; break;
    }
}
}  // namespace

void DisplayOptionsScreen::onInput(const InputEvent& e) {
    if (e.phase != Phase::Press) return;
    switch (e.action) {
        case Action::Menu: ui_.show(ScreenId::Shooting, 0); return;
        case Action::Play: ui_.show(ScreenId::Playback, 0); return;
        case Action::Back: ui_.show(ScreenId::Menu, 7); return;
        case Action::Up: row_ = (row_ + kRows - 1) % kRows; break;
        case Action::Down: row_ = (row_ + 1) % kRows; break;
        case Action::Left: change(ui_.settings, row_, -1); ui_.settingsChanged(); break;
        case Action::Right: case Action::Ok: change(ui_.settings, row_, 1); ui_.settingsChanged(); break;
        default: return;
    }
    ui_.announce(tr(kInfo[row_].label) + " " + valueOf(ui_.settings, row_));
}

void DisplayOptionsScreen::render(Canvas& cv) {
    const Palette& p = palette(ui_.settings.display.style);
    cv.fillRect({0, 0, kLcdW, kLcdH}, p.bg);
    drawHeader(cv, p, 0, tr("title.setup"), tr("disp.title"), std::to_string(row_ + 1) + "/" + std::to_string(kRows));
    const int visible = (kBodyBottom - kBodyTop) / kRowH;
    if (row_ < scroll_) scroll_ = row_;
    if (row_ >= scroll_ + visible) scroll_ = row_ - visible + 1;
    Rect list{0, kBodyTop, 160, kBodyBottom - kBodyTop};
    cv.pushClip(list);
    for (int i = scroll_; i < kRows && i < scroll_ + visible; ++i) {
        RowView v; v.label = tr(kInfo[i].label); v.value = valueOf(ui_.settings, i); v.selected = i == row_;
        drawRow(cv, p, {0, kBodyTop + (i - scroll_) * kRowH, 160, kRowH}, v);
    }
    drawScrollbar(cv, p, list, kRows, visible, scroll_);
    cv.popClip();
    cv.fillRect({159, kBodyTop, 1, list.h}, p.line);

    // live preview: the real overlay drawn on a 320x240 scratch frame, then scaled to 148x111
    static Framebuffer scene;
    static bool haveScene = false;
    if (!haveScene) { renderScene(scene, Scene::Lake, 0); haveScene = true; }
    Framebuffer scratch = scene;
    Canvas sc(scratch);
    HudModel m;
    m.settings = &ui_.settings; m.eff = effective(ui_.settings); m.exposure = ui_.camera.exposure(m.eff);
    m.remaining = ui_.remainingShots(); m.batteryPercent = ui_.camera.status().batteryPercent >= 0 ? ui_.camera.status().batteryPercent : 82;
    m.wall = ui_.wallNow(); m.preview = true;
    m.hist.resize(32);
    for (int i = 0; i < 32; ++i) { float x = float(i - 13) / 7.f; m.hist[size_t(i)] = 0.15f + 0.8f * std::exp(-x * x) * (0.8f + 0.2f * std::sin(float(i) * 1.7f)); }
    drawHud(sc, m);
    cv.blit(scratch, {0, 0, kLcdW, kLcdH}, {166, 30, 148, 111});
    cv.strokeRect({166, 30, 148, 111}, p.line);
    auto lines = wrapText(tr("disp.concept"), 148);
    for (size_t i = 0; i < lines.size(); ++i) cv.text(166, 146 + int(i) * 10, lines[i], style(p.dim));

    drawDescBar(cv, p, tr(kInfo[row_].label), tr(kInfo[row_].descr), false);
    drawHintBar(cv, p, {{"\xE2\x96\xB2\xE2\x96\xBC", "hint.select"}, {"\xE2\x97\x80\xE2\x96\xB6", "hint.change"}, {"Esc", "hint.back"}});
}

}  // namespace s650
