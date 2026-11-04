#include "core/Translation.h"
// Main menu and Setup: port of js/screens/menu.js. Levels: Rail -> Rows -> Picker | Slider | Edit | View.
// MENU always exits to shooting; BACK goes up one level. Last category and row are remembered.
#include <algorithm>
#include <cstdio>
#include "core/Clock.h"
#include "core/DateTime.h"
#include "display/Icons.h"
#include "ui/Screens.h"

namespace s650 {
namespace {
std::string translatedViewKey(const char* group, const std::string& view) {
    FixedKey<64> key(group, ".", view.c_str());
    return key.valid() ? tr(key.c_str()) : std::string();
}
const std::vector<std::string> kCatTitleKeys = {"title.camera", "title.size", "title.focus", "title.flash", "title.exposure", "title.wb", "title.iso", "title.setup"};
const std::vector<std::string> kCatDescKeys = {"dc.camera", "dc.size", "dc.focus", "dc.flash", "dc.exposure", "dc.wb", "dc.iso", "dc.setup"};
}  // namespace

void MenuScreen::enter(int param) {
    if (param >= 0 && param < 8) cat_ = param;
    level_ = Level::Rows; msg_.clear(); msgMs_ = 0;
}

MenuRow& MenuScreen::currentRow(std::vector<MenuCategory>& m) { return m[size_t(cat_)].rows[size_t(rowIdx())]; }

void MenuScreen::flash(const std::string& t, bool warn) { msg_ = t; msgWarn_ = warn; msgMs_ = 1800; ui_.announce(t); }

void MenuScreen::update(uint32_t dt) { if (msgMs_ > 0) { msgMs_ -= int(dt); if (msgMs_ <= 0) msg_.clear(); } }

void MenuScreen::say(const std::vector<MenuCategory>& m) {
    const auto& c = m[size_t(cat_)];
    if (level_ == Level::Rail) { ui_.announce(tr(kCatTitleKeys[size_t(cat_)])); return; }
    if (level_ == Level::Rows) {
        const MenuRow& r = c.rows[size_t(rowIdx())];
        std::string t = r.label + " " + (r.value ? r.value() : "");
        if (!r.disabledReason().empty()) t += ", " + tr(r.disabledReason());
        ui_.announce(t);
    }
}

void MenuScreen::commitEdit() {
    int64_t target = joinDateTime(edit_[0], edit_[1], edit_[2], edit_[3], edit_[4]);
    ui_.settings.setup.clockOffset = int(target - ui_.clock.wallSeconds());
    ui_.settingsChanged();
    level_ = Level::Rows;
}

void MenuScreen::activate(std::vector<MenuCategory>& m, MenuRow& r) {
    std::string why = r.disabledReason();
    if (!why.empty()) { flash(tr(why), true); return; }
    switch (r.type) {
        case RowType::Choice: {
            auto opts = r.options();
            pickOriginal_ = r.current(); pickIndex_ = 0;
            for (size_t i = 0; i < opts.size(); ++i) if (opts[i].id == pickOriginal_) pickIndex_ = int(i);
            level_ = Level::Picker; say(m); break; }
        case RowType::Bool: r.apply(r.current() ? 0 : 1); ui_.settingsChanged(); say(m); break;
        case RowType::Slider: sliderIndex_ = r.current(); sliderOriginal_ = sliderIndex_; level_ = Level::Slider; break;
        case RowType::DateTime: {
            int y, mo, d, h, mi; splitDateTime(ui_.wallNow(), y, mo, d, h, mi);
            edit_[0] = y; edit_[1] = mo; edit_[2] = d; edit_[3] = h; edit_[4] = mi; editField_ = 0; level_ = Level::Edit; break; }
        case RowType::Action:
            switch (r.action) {
                case ActionId::OpenDisplay: ui_.show(ScreenId::Display, 0); break;
                case ActionId::ViewMemory: view_ = "memory"; level_ = Level::View; break;
                case ActionId::ViewStatus: view_ = "status"; level_ = Level::View; break;
                case ActionId::Initialize: {
                    Dialog d; d.title = tr("dlg.init.title"); d.message = tr("dlg.init.msg"); d.ok = tr("btn.ok"); d.cancel = tr("btn.cancel"); d.focusOk = true;
                    d.onOk = [this] { initializeSettings(ui_.settings); ui_.settingsChanged(); flash(tr("msg.initialized"), false); };
                    ui_.confirm(d); break; }
                case ActionId::Format: {
                    Dialog d; d.title = tr("dlg.format.title"); d.message = tr("dlg.format.msg"); d.ok = tr("btn.format"); d.cancel = tr("btn.cancel"); d.danger = true;
                    d.onOk = [this] {
                        int n = ui_.storage.format();
                        std::string t = tr("msg.formatted"); auto p = t.find("%n"); if (p != std::string::npos) t.replace(p, 2, std::to_string(n));
                        flash(t, false); };
                    ui_.confirm(d); break; }
                default: break;
            }
            break;
    }
}

void MenuScreen::onInput(const InputEvent& e) {
    if (e.phase != Phase::Press) return;
    Action a = e.action;
    bool classic = palette(ui_.settings.display.style).classic;
    auto menu = buildMenu(ui_.settings, ui_.storage, ui_.wallNow());
    msg_.clear(); msgMs_ = 0;
    if (a == Action::Menu) { ui_.show(ScreenId::Shooting, 0); return; }
    if (a == Action::Play) { ui_.show(ScreenId::Playback, 0); return; }
    if (a == Action::Shutter || a == Action::Wide || a == Action::Tele || a == Action::ModePrev || a == Action::ModeNext || a == Action::Delete || a == Action::Power || a == Action::Protect) return;
    int nCats = int(menu.size());
    auto nRows = [&] { return int(menu[size_t(cat_)].rows.size()); };

    switch (level_) {
    case Level::Rail:
        if (!classic) {
            if (a == Action::Up) cat_ = (cat_ + nCats - 1) % nCats; else if (a == Action::Down) cat_ = (cat_ + 1) % nCats;
            else if (a == Action::Right || a == Action::Ok) level_ = Level::Rows; else if (a == Action::Back) { ui_.show(ScreenId::Shooting, 0); return; }
        } else {
            if (a == Action::Left) cat_ = (cat_ + nCats - 1) % nCats; else if (a == Action::Right) cat_ = (cat_ + 1) % nCats;
            else if (a == Action::Down || a == Action::Ok) level_ = Level::Rows; else if (a == Action::Back) { ui_.show(ScreenId::Shooting, 0); return; }
        }
        say(menu); break;
    case Level::Rows: {
        MenuRow& r = currentRow(menu);
        if (a == Action::Up) { if (classic && rowIdx() == 0) level_ = Level::Rail; else rows_[cat_] = (rowIdx() + nRows() - 1) % nRows(); say(menu); }
        else if (a == Action::Down) { rows_[cat_] = (rowIdx() + 1) % nRows(); say(menu); }
        else if (a == Action::Ok || a == Action::Right) activate(menu, r);
        else if (a == Action::Back || (a == Action::Left && !classic)) { level_ = Level::Rail; say(menu); }
        break; }
    case Level::Picker: {
        MenuRow& r = currentRow(menu);
        auto opts = r.options(); int n = int(opts.size());
        if (a == Action::Up) { pickIndex_ = (pickIndex_ + n - 1) % n; say(menu); }
        else if (a == Action::Down) { pickIndex_ = (pickIndex_ + 1) % n; say(menu); }
        else if (a == Action::Ok) { r.apply(opts[size_t(pickIndex_)].id); ui_.settingsChanged(); level_ = Level::Rows; say(menu); }
        else if (a == Action::Left || a == Action::Back) level_ = Level::Rows;
        break; }
    case Level::Slider: {
        MenuRow& r = currentRow(menu);
        if (a == Action::Left || a == Action::Right) {
            sliderIndex_ = std::clamp(sliderIndex_ + (a == Action::Right ? 1 : -1), 0, r.sliderCount - 1);
            r.apply(sliderIndex_); ui_.settingsChanged();
        } else if (a == Action::Ok) { level_ = Level::Rows; say(menu); }
        else if (a == Action::Back) { r.apply(sliderOriginal_); ui_.settingsChanged(); level_ = Level::Rows; }
        break; }
    case Level::Edit: {
        static const int lo[5] = {2000, 1, 1, 0, 0}, hi[5] = {2099, 12, 31, 23, 59};
        if (a == Action::Left) editField_ = (editField_ + 4) % 5; else if (a == Action::Right) editField_ = (editField_ + 1) % 5;
        else if (a == Action::Up || a == Action::Down) {
            int v = edit_[editField_] + (a == Action::Up ? 1 : -1);
            if (v > hi[editField_]) v = lo[editField_];
            if (v < lo[editField_]) v = hi[editField_];
            edit_[editField_] = v;
        } else if (a == Action::Ok) commitEdit();
        else if (a == Action::Back) level_ = Level::Rows;
        break; }
    case Level::View:
        if (a == Action::Back || a == Action::Ok || a == Action::Left) level_ = Level::Rows;
        break;
    }
}

void MenuScreen::render(Canvas& cv) {
    const Palette& p = palette(ui_.settings.display.style);
    auto menu = buildMenu(ui_.settings, ui_.storage, ui_.wallNow());
    bool classic = p.classic;
    const MenuCategory& cat = menu[size_t(cat_)];
    cv.fillRect({0, 0, kLcdW, kLcdH}, p.bg);

    // pane rectangle and visible row count
    Rect pane = classic ? Rect{0, kClassicTabsH + kHdrH, kLcdW, 5 * kRowH} : Rect{kRailW, kBodyTop, kLcdW - kRailW, kBodyBottom - kBodyTop};
    int visible = pane.h / kRowH;
    bool railFocus = level_ == Level::Rail;

    // header
    std::string title = tr(kCatTitleKeys[size_t(cat_)]), meta;
    const MenuRow* row = cat.rows.empty() ? nullptr : &cat.rows[size_t(rowIdx())];
    if (level_ == Level::Picker && row) { if (row->label != title) title += "  \xE2\x80\xBA  " + row->label; meta = std::to_string(pickIndex_ + 1) + "/" + std::to_string(row->options().size()); }
    else if ((level_ == Level::Slider || level_ == Level::Edit) && row) { if (row->label != title) title += "  \xE2\x80\xBA  " + row->label; }
    else if (level_ == Level::View) title = tr("title.setup") + "  \xE2\x80\xBA  " + translatedViewKey("row", view_);
    else if (cat.rows.size() > 1) meta = std::to_string(rowIdx() + 1) + "/" + std::to_string(cat.rows.size());
    drawHeader(cv, p, classic ? kClassicTabsH : 0, tr("hdr.menu"), title, meta);

    // rail / tabs
    if (classic) {
        cv.fillRect({0, 0, kLcdW, kClassicTabsH}, p.head);
        for (int i = 0; i < 8; ++i) {
            Rect t{i * 40, 0, 40, kClassicTabsH};
            if (i == cat_) {
                if (railFocus) cv.fillRect(t, p.accent); else { cv.fillRect(t, p.panel); cv.fillRect({t.x, t.y + t.h - 3, t.w, 3}, p.accent); }
            }
            drawIcon(cv, categoryIcon(i), t.x + 12, 7, 16, i == cat_ ? (railFocus ? p.accentInk : p.ink) : p.dim);
        }
    } else {
        cv.fillRect({0, kBodyTop, kRailW, kBodyBottom - kBodyTop}, p.panel);
        cv.fillRect({kRailW - 1, kBodyTop, 1, kBodyBottom - kBodyTop}, p.line);
        for (int i = 0; i < 8; ++i) {
            Rect t{0, kBodyTop + i * 21, kRailW - 1, 21};
            if (i == cat_) { cv.fillRect(t, p.panel2); if (railFocus) cv.fillRect({0, t.y, 2, t.h}, p.accent); }
            drawIcon(cv, categoryIcon(i), 11, t.y + 3, 15, i == cat_ ? (railFocus ? p.accent : p.ink) : p.dim);
        }
    }

    cv.pushClip(pane);
    if (level_ == Level::Picker && row) {
        auto opts = row->options();
        int& sc = scroll_[cat_];
        if (pickIndex_ < sc) sc = pickIndex_; if (pickIndex_ >= sc + visible) sc = pickIndex_ - visible + 1;
        for (size_t i = size_t(sc); i < opts.size() && int(i) < sc + visible; ++i) {
            Rect r{pane.x, pane.y + (int(i) - sc) * kRowH, pane.w, kRowH};
            bool sel = int(i) == pickIndex_, cur = opts[i].id == pickOriginal_;
            Color text = p.ink, extra = p.dim;
            if (sel) { if (classic) { cv.fillRect(r, p.accent); text = extra = p.accentInk; } else { cv.fillRect(r, p.panel2); cv.fillRect({r.x, r.y, 2, r.h}, p.accent); extra = p.accent; } }
            cv.fillRect({r.x, r.y + r.h - 1, r.w, 1}, sel && classic ? p.accent : p.line);
            Color ring = cur ? (sel && classic ? p.accentInk : p.accent) : p.faint;
            cv.circle(float(r.x + 16), float(r.y + 12), 4.5f, 1.5f, ring);
            if (cur) cv.fillCircle(float(r.x + 16), float(r.y + 12), 2.2f, ring);
            cv.text(r.x + 28, r.y + 8, opts[i].label, style(text, sel && classic));
            if (!opts[i].extra.empty()) cv.textRight(r.x + r.w - 10, r.y + 8, opts[i].extra, style(extra));
        }
        drawScrollbar(cv, p, pane, int(opts.size()), visible, sc);
    } else if (level_ == Level::Slider && row) {
        int n = row->sliderCount;
        std::string big = row->sliderFmt(sliderIndex_);
        cv.textCenter(pane.x + pane.w / 2, pane.y + pane.h / 2 - 36, big, style(p.accent, true, 2));
        int tw = 210, tx = pane.x + (pane.w - tw) / 2, ty = pane.y + pane.h / 2 + 2;
        cv.fillRect({tx, ty + 7, tw, 1}, p.faint);
        for (int i = 0; i < n; ++i) {
            int x = tx + (n > 1 ? i * (tw - 1) / (n - 1) : 0);
            bool major = i % std::max(1, row->sliderMajor) == 0;
            cv.fillRect({x, ty + (major ? 2 : 4), 1, major ? 11 : 7}, major ? p.dim : p.faint);
        }
        int mx = tx + (n > 1 ? sliderIndex_ * (tw - 1) / (n - 1) : 0);
        cv.fillRect({mx - 3, ty, 6, 14}, p.accent);
        cv.text(tx, ty + 20, row->lo, style(p.dim)); cv.textRight(tx + tw, ty + 20, row->hi, style(p.dim));
    } else if (level_ == Level::View) {
        auto kv = [&](int y, const std::string& k, const std::string& v) { cv.text(pane.x + 12, y, k, style(p.dim)); cv.textRight(pane.x + pane.w - 12, y, v, style(p.ink)); };
        auto cap = [&](int y, const std::string& t) { cv.text(pane.x + 12, y, t, style(p.dim, true)); };
        int y = pane.y + 8;
        if (view_ == "memory") {
            cap(y, tr("mem.card")); y += 13;
            Rect bar{pane.x + 12, y, pane.w - 24, 6};
            cv.fillRect(bar, p.panel2); cv.strokeRect(bar, p.line);
            double frac = ui_.storage.capacityBytes() ? double(ui_.storage.usedBytes()) / double(ui_.storage.capacityBytes()) : 0;
            cv.fillRect({bar.x + 1, bar.y + 1, std::max(1, int((bar.w - 2) * frac)), 4}, p.accent); y += 12;
            auto fmt = [](uint64_t b) { char s[24]; if (b >= 1000000000ull) std::snprintf(s, sizeof s, "%.2f GB", double(b) / 1e9); else std::snprintf(s, sizeof s, "%.1f MB", double(b) / 1e6); return std::string(s); };
            kv(y, tr("mem.used"), fmt(ui_.storage.usedBytes())); y += 12;
            kv(y, tr("mem.free"), fmt(ui_.storage.freeBytes())); y += 12;
            kv(y, tr("mem.shots"), std::to_string(ui_.storage.images().size())); y += 12;
            kv(y, tr("mem.left"), std::to_string(ui_.remainingShots())); y += 18;
            cap(y, tr("mem.internal")); y += 13;
            kv(y, "\xE2\x89\x88 " + fmt(ui_.storage.internalBytes()), "");
        } else {
            CameraStatus cs = ui_.camera.status();
            cap(y, tr("status.concept")); y += 13;
            kv(y, tr("status.version"), "Native 0.1"); y += 12;
            kv(y, tr("status.firmware"), tr("status.notreal")); y += 18;
            cap(y, cs.simulated ? tr("status.camera") : std::string("Camera")); y += 13;
            kv(y, "Backend", cs.simulated ? "simulated" : cs.available ? "hardware" : "unavailable"); y += 12;
            kv(y, tr("status.battery"), cs.batteryPercent >= 0 ? std::to_string(cs.batteryPercent) + " %" : "--"); y += 12;
            kv(y, tr("status.clock"), formatDateTime(ui_.wallNow())); y += 12;
            kv(y, tr("status.images"), std::to_string(ui_.storage.images().size()));
        }
    } else {
        int& sc = scroll_[cat_];
        int sel = rowIdx();
        if (sel < sc) sc = sel; if (sel >= sc + visible) sc = sel - visible + 1;
        for (size_t i = size_t(sc); i < cat.rows.size() && int(i) < sc + visible; ++i) {
            const MenuRow& r = cat.rows[i];
            RowView v;
            v.label = r.label; v.value = r.value ? r.value() : ""; v.selected = int(i) == sel && !railFocus;
            v.disabled = !r.disabledReason().empty(); v.danger = r.danger;
            v.chevron = r.type != RowType::Bool && !(r.type == RowType::Action && false);
            Rect rr{pane.x, pane.y + (int(i) - sc) * kRowH, pane.w, kRowH};
            if (level_ == Level::Edit && int(i) == sel) {
                drawRow(cv, p, rr, [&] { RowView e = v; e.value.clear(); e.chevron = false; return e; }());
                // five editable fields right-aligned
                char buf[5][8]; std::snprintf(buf[0], 8, "%04d", edit_[0]);
                for (int k = 1; k < 5; ++k) std::snprintf(buf[k], 8, "%02d", edit_[k]);
                const char* sep[5] = {"-", "-", " ", ":", ""};
                int total = 0; for (int k = 0; k < 5; ++k) total += Canvas::textWidth(buf[k]) + 6 + Canvas::textWidth(sep[k]) + 1;
                int x = rr.x + rr.w - 10 - total;
                for (int k = 0; k < 5; ++k) {
                    int w = Canvas::textWidth(buf[k]);
                    if (k == editField_) cv.fillRect({x - 2, rr.y + 5, w + 4, 14}, p.accent); else cv.fillRect({x - 2, rr.y + 5, w + 4, 14}, p.ink, 36);
                    cv.text(x, rr.y + 8, buf[k], style(k == editField_ ? p.accentInk : (v.selected && classic ? p.accentInk : p.ink), k == editField_));
                    x += w + 6; x += cv.text(x - 2, rr.y + 8, sep[k], style(p.dim)) + 1;
                }
            } else drawRow(cv, p, rr, v);
        }
        drawScrollbar(cv, p, pane, int(cat.rows.size()), visible, sc);
    }
    cv.popClip();

    // description bar
    std::string name, text; bool warn = false;
    if (!msg_.empty()) { name = msg_; warn = msgWarn_; }
    else if (level_ == Level::Rail) { name = tr(kCatTitleKeys[size_t(cat_)]); text = tr(kCatDescKeys[size_t(cat_)]); }
    else if (level_ == Level::View) text = translatedViewKey("dr.view", view_);
    else if (level_ == Level::Picker && row) { auto o = row->options()[size_t(pickIndex_)]; name = o.label; text = o.descr.empty() ? row->descr : o.descr; }
    else if (level_ == Level::Edit && row) { name = row->label; text = tr("dr.datetime.edit"); }
    else if (row) {
        std::string why = level_ == Level::Rows ? row->disabledReason() : "";
        name = row->label; text = why.empty() ? row->descr : tr(why); warn = !why.empty();
    }
    drawDescBar(cv, p, name, text, warn);

    std::vector<Hint> h;
    switch (level_) {
        case Level::Rail: h = classic ? std::vector<Hint>{{"\xE2\x97\x80\xE2\x96\xB6", "hint.select"}, {"\xE2\x96\xBC", "hint.open"}, {"M", "hint.exit"}}
                                      : std::vector<Hint>{{"\xE2\x96\xB2\xE2\x96\xBC", "hint.select"}, {"\xE2\x96\xB6", "hint.open"}, {"M", "hint.exit"}}; break;
        case Level::Rows: h = {{"\xE2\x96\xB2\xE2\x96\xBC", "hint.select"}, {"OK", "hint.choose"}, {classic ? "Esc" : "\xE2\x97\x80", "hint.back"}, {"M", "hint.exit"}}; break;
        case Level::Picker: h = {{"\xE2\x96\xB2\xE2\x96\xBC", "hint.select"}, {"OK", "hint.set"}, {"Esc", "hint.cancel"}}; break;
        case Level::Slider: h = {{"\xE2\x97\x80\xE2\x96\xB6", "hint.adjust"}, {"OK", "hint.set"}, {"Esc", "hint.cancel"}}; break;
        case Level::Edit: h = {{"\xE2\x97\x80\xE2\x96\xB6", "hint.field"}, {"\xE2\x96\xB2\xE2\x96\xBC", "hint.value"}, {"OK", "hint.confirm"}, {"Esc", "hint.cancel"}}; break;
        case Level::View: h = {{"Esc", "hint.back"}}; break;
    }
    drawHintBar(cv, p, h);
}

}  // namespace s650
