// Playback (port of js/screens/playback.js): 4x3 index, viewer with info levels and zoom, playback menu,
// protect, delete with confirmation, simulated slideshow with a cross-fade.
#include <algorithm>
#include <cstdio>
#include "core/Clock.h"
#include "core/DateTime.h"
#include "display/Icons.h"
#include "ui/Screens.h"

namespace s650 {
namespace {
constexpr int kPerPage = 12, kCols = 4;
}

void PlaybackScreen::enter(int param) {
    mode_ = Mode::Index; zoom_ = 1; px_ = py_ = 0; info_ = 0; menuOpen_ = false; toast_.clear(); toastMs_ = 0; fadeMs_ = -1;
    if (param == 1) sel_ = int(ui_.storage.images().size()) - 1;
    sel_ = std::clamp(sel_, 0, std::max(0, int(ui_.storage.images().size()) - 1));
}
void PlaybackScreen::exit() { menuOpen_ = false; }

void PlaybackScreen::toast(const std::string& t) { toast_ = t; toastMs_ = 1700; ui_.announce(t); }

void PlaybackScreen::stopSlideshow(const std::string& msg) {
    if (mode_ != Mode::Slideshow) return;
    mode_ = Mode::View; fadeMs_ = -1;
    if (!msg.empty()) toast(msg);
}

void PlaybackScreen::update(uint32_t dt) {
    if (toastMs_ > 0) { toastMs_ -= int(dt); if (toastMs_ <= 0) toast_.clear(); }
    if (mode_ != Mode::Slideshow) return;
    int n = int(ui_.storage.images().size());
    if (n == 0) { stopSlideshow(""); return; }
    if (fadeMs_ >= 0) {
        fadeMs_ += int(dt);
        if (fadeMs_ >= 280) { sel_ = fadeTo_; fadeMs_ = -1; ssMs_ = 0; }
        return;
    }
    ssMs_ += int(dt);
    if (ssMs_ >= kSlideIntervals[ui_.settings.playback.interval] * 1000) {
        int nx = sel_ + 1;
        if (nx >= n) { if (ui_.settings.playback.repeat) nx = 0; else { stopSlideshow(tr("msg.ssend")); return; } }
        fadeTo_ = nx; fadeMs_ = 0;
    }
}

void PlaybackScreen::askDelete() {
    auto& imgs = ui_.storage.images();
    if (imgs.empty()) return;
    const ImageRecord& r = imgs[size_t(sel_)];
    if (r.isProtected) { toast(tr("msg.isprotected")); return; }
    Dialog d; d.title = tr("dlg.del.title"); d.message = tr("dlg.del.msg") + "  " + r.name; d.ok = tr("btn.delete"); d.cancel = tr("btn.cancel"); d.danger = true;
    size_t idx = size_t(sel_);
    d.onOk = [this, idx] {
        Result res = ui_.storage.removeImage(idx);
        if (!res.ok) toast(res.error);
        else { toast(tr("msg.deleted")); int n = int(ui_.storage.images().size()); sel_ = std::clamp(sel_, 0, std::max(0, n - 1)); if (n == 0) mode_ = Mode::Index; }
    };
    ui_.confirm(d);
}

void PlaybackScreen::askDeleteAll() {
    Dialog d; d.title = tr("dlg.del.title"); d.message = tr("dlg.delall.msg"); d.ok = tr("btn.delete"); d.cancel = tr("btn.cancel"); d.danger = true;
    d.onOk = [this] {
        int n = ui_.storage.removeAllUnprotected();
        std::string t = tr("msg.deletedn"); auto p = t.find("%n"); if (p != std::string::npos) t.replace(p, 2, std::to_string(n));
        toast(t); sel_ = 0; if (ui_.storage.images().empty()) mode_ = Mode::Index;
    };
    ui_.confirm(d);
}

void PlaybackScreen::toggleProtect() {
    auto& imgs = ui_.storage.images();
    if (imgs.empty()) return;
    bool on = !imgs[size_t(sel_)].isProtected;
    Result r = ui_.storage.setProtected(size_t(sel_), on);
    toast(r.ok ? tr(on ? "msg.protected" : "msg.unprotected") : r.error);
}

void PlaybackScreen::menuInput(Action a) {
    auto& pb = ui_.settings.playback;
    auto& imgs = ui_.storage.images();
    const int rows = 6;
    bool prot = !imgs.empty() && imgs[size_t(sel_)].isProtected;
    if (a == Action::Up) menuRow_ = (menuRow_ + rows - 1) % rows;
    else if (a == Action::Down) menuRow_ = (menuRow_ + 1) % rows;
    else if (a == Action::Back || a == Action::Menu || (a == Action::Left && menuRow_ != 1)) menuOpen_ = false;
    else if (a == Action::Ok || a == Action::Right || a == Action::Left) {
        if (menuRow_ == 0) { menuOpen_ = false; mode_ = Mode::Slideshow; zoom_ = 1; px_ = py_ = 0; ssMs_ = 0; fadeMs_ = -1; }
        else if (menuRow_ == 1) { pb.interval = (pb.interval + (a == Action::Left ? 2 : 1)) % 3; ui_.settingsChanged(); }
        else if (menuRow_ == 2) { pb.repeat = !pb.repeat; ui_.settingsChanged(); }
        else if (a == Action::Ok && menuRow_ == 3) { menuOpen_ = false; toggleProtect(); }
        else if (a == Action::Ok && menuRow_ == 4) { menuOpen_ = false; if (prot) toast(tr("msg.isprotected")); else askDelete(); }
        else if (a == Action::Ok && menuRow_ == 5) { menuOpen_ = false; askDeleteAll(); }
    }
}

void PlaybackScreen::onInput(const InputEvent& e) {
    if (e.phase != Phase::Press) return;
    Action a = e.action;
    int n = int(ui_.storage.images().size());
    if (mode_ == Mode::Slideshow) { stopSlideshow(""); return; }
    if (menuOpen_) { menuInput(a); return; }
    if (a == Action::Play) { ui_.show(ScreenId::Shooting, 0); return; }
    if (a == Action::Shutter || a == Action::ModeNext || a == Action::ModePrev || a == Action::Power) return;
    if (a == Action::Menu) { if (n) { menuOpen_ = true; menuRow_ = 0; } else ui_.show(ScreenId::Menu, -1); return; }
    if (n == 0) { if (a == Action::Back) ui_.show(ScreenId::Shooting, 0); return; }
    if (a == Action::Delete) { askDelete(); return; }
    if (a == Action::Protect) { toggleProtect(); return; }

    if (mode_ == Mode::Index) {
        if (a == Action::Left) sel_ = std::max(0, sel_ - 1);
        else if (a == Action::Right) sel_ = std::min(n - 1, sel_ + 1);
        else if (a == Action::Up) sel_ = std::max(0, sel_ - kCols);
        else if (a == Action::Down) sel_ = std::min(n - 1, sel_ + kCols);
        else if (a == Action::Ok || a == Action::Tele) { mode_ = Mode::View; zoom_ = 1; px_ = py_ = 0; }
        else if (a == Action::Back) { ui_.show(ScreenId::Shooting, 0); return; }
        return;
    }
    if (zoom_ > 1) {
        int mx = (kLcdW - kLcdW / zoom_) / 2, my = (kLcdH - kLcdH / zoom_) / 2;
        if (a == Action::Left) px_ = std::max(-mx, px_ - 24); else if (a == Action::Right) px_ = std::min(mx, px_ + 24);
        else if (a == Action::Up) py_ = std::max(-my, py_ - 24); else if (a == Action::Down) py_ = std::min(my, py_ + 24);
        else if (a == Action::Tele) zoom_ = std::min(4, zoom_ * 2);
        else if (a == Action::Wide) { zoom_ = std::max(1, zoom_ / 2); if (zoom_ == 1) px_ = py_ = 0; else { int m2x = (kLcdW - kLcdW / zoom_) / 2, m2y = (kLcdH - kLcdH / zoom_) / 2; px_ = std::clamp(px_, -m2x, m2x); py_ = std::clamp(py_, -m2y, m2y); } }
        else if (a == Action::Back) { zoom_ = 1; px_ = py_ = 0; }
        return;
    }
    if (a == Action::Left) sel_ = (sel_ + n - 1) % n;
    else if (a == Action::Right) sel_ = (sel_ + 1) % n;
    else if (a == Action::Up) info_ = (info_ + 1) % 3;
    else if (a == Action::Down) info_ = (info_ + 2) % 3;
    else if (a == Action::Tele || a == Action::Ok) zoom_ = 2;
    else if (a == Action::Wide || a == Action::Back) mode_ = Mode::Index;
}

void PlaybackScreen::drawBig(Canvas& cv, size_t idx, int alpha) {
    const ImageRecord& r = ui_.storage.images()[idx];
    if (!r.pixels) return;
    int sw = kLcdW / zoom_, sh = kLcdH / zoom_;
    int sx = std::clamp((kLcdW - sw) / 2 + px_, 0, kLcdW - sw), sy = std::clamp((kLcdH - sh) / 2 + py_, 0, kLcdH - sh);
    cv.blit(*r.pixels, {sx, sy, sw, sh}, {0, 0, kLcdW, kLcdH}, alpha);
}

void PlaybackScreen::renderIndex(Canvas& cv) {
    const Palette& p = palette(ui_.settings.display.style);
    auto& imgs = ui_.storage.images();
    int n = int(imgs.size()), page = sel_ / kPerPage, pages = std::max(1, (n + kPerPage - 1) / kPerPage);
    cv.fillRect({0, 0, kLcdW, kLcdH}, p.bg);
    std::string meta = n ? std::to_string(sel_ + 1) + " / " + std::to_string(n) + "  \xC2\xB7  " + std::to_string(page + 1) + "/" + std::to_string(pages) : "";
    drawHeader(cv, p, 0, tr("pb.title"), n ? imgs[size_t(sel_)].folder : "", meta);
    if (n == 0) {
        drawIcon(cv, Icon::Images, 146, 96, 28, p.faint);
        cv.textCenter(160, 132, tr("pb.noimages"), style(p.dim));
    } else {
        for (int i = page * kPerPage; i < std::min(n, (page + 1) * kPerPage); ++i) {
            int k = i - page * kPerPage;
            int gx = (kLcdW - (4 * 72 + 3 * 5)) / 2 + (k % kCols) * 77, gy = 27 + (k / kCols) * 58;
            Rect r{gx, gy, 72, 54};
            if (imgs[size_t(i)].pixels) cv.blit(*imgs[size_t(i)].pixels, {0, 0, kLcdW, kLcdH}, r);
            cv.strokeRect(r, p.line);
            if (i == sel_) cv.strokeRect({r.x - 1, r.y - 1, r.w + 2, r.h + 2}, p.accent, 2);
            if (imgs[size_t(i)].isProtected) { cv.fillRect({r.x + r.w - 14, r.y + 2, 12, 12}, Color(0, 0, 0), 153); drawIcon(cv, Icon::Lock, r.x + r.w - 13, r.y + 3, 10, Color(255, 255, 255)); }
        }
        const ImageRecord& r = imgs[size_t(sel_)];
        Rect strip{0, kLcdH - kHintH - 22, kLcdW, 22};
        cv.fillRect(strip, p.bg2); cv.fillRect({0, strip.y, kLcdW, 1}, p.line);
        int w = cv.text(8, strip.y + 8, r.name, style(p.ink, true));
        cv.text(8 + w + 12, strip.y + 8, formatDateTime(r.taken) + "  " + std::to_string(r.w) + "\xC3\x97" + std::to_string(r.h), style(p.dim));
        if (r.isProtected) drawIcon(cv, Icon::Lock, kLcdW - 22, strip.y + 5, 12, p.accent);
    }
    drawHintBar(cv, p, n ? std::vector<Hint>{{"\xE2\x96\xB2\xE2\x96\xBC\xE2\x97\x80\xE2\x96\xB6", "hint.select"}, {"OK", "hint.view"}, {"M", "hint.menu"}, {"P", "hint.shooting"}}
                         : std::vector<Hint>{{"P", "hint.shooting"}});
}

void PlaybackScreen::renderView(Canvas& cv) {
    const Palette& p = palette(ui_.settings.display.style);
    auto& imgs = ui_.storage.images();
    const ImageRecord& r = imgs[size_t(sel_)];
    cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0));
    drawBig(cv, size_t(sel_), 255);
    if (mode_ == Mode::Slideshow && fadeMs_ >= 0) { int s = sel_; sel_ = fadeTo_; drawBig(cv, size_t(fadeTo_), std::min(255, fadeMs_ * 255 / 280)); sel_ = s; }
    TextStyle ts = style(Color(255, 255, 255)); ts.shadow = true;
    cv.fillRect({0, 0, kLcdW, 20}, Color(6, 7, 8), 128);
    cv.text(7, 6, r.name, ts);
    int rx = kLcdW - 7;
    rx -= cv.textRight(rx, 6, std::to_string(sel_ + 1) + " / " + std::to_string(imgs.size()), ts) + 8;
    if (r.isProtected) drawIcon(cv, Icon::Lock, rx - 13, 3, 13, Color(255, 178, 77));
    if (zoom_ > 1) {
        cv.fillRect({8, 26, 24, 14}, Color(6, 7, 8), 180); cv.text(13, 30, "\xC3\x97" + std::to_string(zoom_), ts);
        Rect mini{kLcdW - 8 - 40, 26, 40, 30};
        cv.fillRect(mini, Color(6, 7, 8), 140); cv.strokeRect(mini, Color(255, 255, 255), 1, 180);
        float vw = 40.f / float(zoom_), vh = 30.f / float(zoom_);
        float mx = float(mini.x) + 20.f + float(px_) / float(kLcdW) * 40.f - vw / 2, my = float(mini.y) + 15.f + float(py_) / float(kLcdH) * 30.f - vh / 2;
        cv.strokeRect({int(mx), int(my), std::max(3, int(vw)), std::max(3, int(vh))}, Color(255, 178, 77), 1);
    }
    if (info_ > 0 && mode_ != Mode::Slideshow && zoom_ == 1) {
        int lines = 3, h = lines * 12 + 12;
        cv.fillRect({0, kLcdH - kHintH - h, kLcdW, h}, Color(6, 7, 8), 204);
        int y = kLcdH - kHintH - h + 7;
        auto kv = [&](int x, int w, int yy, const std::string& k, const std::string& v) { cv.text(x, yy, k, style(Color(170, 177, 185))); cv.textRight(x + w, yy, v, style(Color(238, 240, 242))); };
        int colW = info_ > 1 ? 140 : 300;
        kv(10, colW, y, tr("pb.date"), formatDateTime(r.taken));
        kv(10, colW, y + 12, tr("pb.size"), std::to_string(r.w) + "\xC3\x97" + std::to_string(r.h) + (r.fine ? " FINE" : " STD"));
        char fs[24]; std::snprintf(fs, sizeof fs, "%.1f MB", double(r.bytes) / 1e6);
        kv(10, colW, y + 24, tr("pb.filesize"), fs);
        if (info_ > 1) {
            kv(170, 140, y, tr("pb.iso"), std::to_string(r.meta.iso));
            kv(170, 140, y + 12, tr("pb.exposure"), r.meta.shutter + " " + r.meta.aperture);
            kv(170, 140, y + 24, tr("pb.focal"), std::to_string(r.meta.focalMM) + " mm");
        }
    }
    if (mode_ == Mode::Slideshow) cv.textCenter(160, 226, tr("ss.playing"), ts);
    else drawHintBar(cv, p, zoom_ > 1 ? std::vector<Hint>{{"\xE2\x96\xB2\xE2\x96\xBC\xE2\x97\x80\xE2\x96\xB6", "hint.pan"}, {"T/W", "hint.zoom"}, {"Esc", "hint.back"}}
                                      : std::vector<Hint>{{"\xE2\x97\x80\xE2\x96\xB6", "hint.photo"}, {"\xE2\x96\xB2", "hint.info"}, {"T", "hint.zoom"}, {"W", "hint.index"}, {"M", "hint.menu"}}, true);
}

void PlaybackScreen::render(Canvas& cv) {
    const Palette& p = palette(ui_.settings.display.style);
    int n = int(ui_.storage.images().size());
    if (n == 0) { mode_ = Mode::Index; menuOpen_ = false; }
    sel_ = std::clamp(sel_, 0, std::max(0, n - 1));
    if (mode_ == Mode::Index) renderIndex(cv); else renderView(cv);
    if (menuOpen_ && n) {
        Rect m{110, 26, 204, 20 + 6 * kRowH};
        cv.fillRect(m, p.panel); cv.strokeRect(m, p.line);
        cv.fillRect({m.x + 1, m.y + 1, m.w - 2, 19}, p.head);
        cv.text(m.x + 8, m.y + 7, tr("pb.menu"), style(p.dim));
        const ImageRecord& r = ui_.storage.images()[size_t(sel_)];
        const auto& pb = ui_.settings.playback;
        struct R { std::string l, v; bool danger, off; } rows[6] = {
            {tr("pm.slideshow"), "", false, false}, {tr("pm.interval"), std::to_string(kSlideIntervals[pb.interval]) + " " + tr("pm.sec"), false, false},
            {tr("pm.repeat"), tr(pb.repeat ? "opt.on" : "opt.off"), false, false}, {tr(r.isProtected ? "pm.unprotect" : "pm.protect"), "", false, false},
            {tr("pm.delete"), "", true, r.isProtected}, {tr("pm.deleteall"), "", true, false}};
        for (int i = 0; i < 6; ++i) {
            RowView v; v.label = rows[i].l; v.value = rows[i].v; v.selected = i == menuRow_; v.danger = rows[i].danger; v.disabled = false;
            Rect rr{m.x + 1, m.y + 20 + i * kRowH, m.w - 2, kRowH};
            drawRow(cv, p, rr, v);
            if (rows[i].off) cv.fillRect(rr, p.panel, 120);
        }
    }
    if (!toast_.empty()) {
        int w = Canvas::textWidth(toast_) + 20;
        cv.fillRect({(kLcdW - w) / 2, 26, w, 17}, Color(6, 7, 8), 209);
        cv.textCenter(kLcdW / 2, 31, toast_, style(Color(255, 255, 255)));
    }
}

}  // namespace s650
