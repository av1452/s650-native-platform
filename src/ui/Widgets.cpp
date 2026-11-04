#include "ui/Widgets.h"
#include <algorithm>
#include "core/Strings.h"
#include "display/Icons.h"

namespace s650 {

TextStyle style(const Color& c, bool bold, int scale, int alpha) {
    TextStyle s; s.color = c; s.bold = bold; s.scale = scale; s.alpha = uint8_t(alpha); return s;
}

void drawHeader(Canvas& cv, const Palette& p, int y, const std::string& crumb, const std::string& title, const std::string& meta) {
    cv.fillRect({0, y, kLcdW, kHdrH}, p.head);
    cv.fillRect({0, y + kHdrH - 1, kLcdW, 1}, p.line);
    int x = 8;
    x += cv.text(x, y + 7, crumb, style(p.dim)) + 4;
    x += cv.text(x, y + 7, "\xE2\x80\xBA", style(p.faint)) + 5;
    cv.text(x, y + 7, title, style(p.ink, true));
    if (!meta.empty()) cv.textRight(kLcdW - 8, y + 7, meta, style(p.dim));
}

void drawHintBar(Canvas& cv, const Palette& p, const std::vector<Hint>& hints, bool translucent) {
    Rect r{0, kLcdH - kHintH, kLcdW, kHintH};
    if (translucent) cv.fillRect(r, Color(6, 7, 8), 184); else { cv.fillRect(r, p.head); cv.fillRect({0, r.y, kLcdW, 1}, p.line); }
    int x = 8;
    for (const auto& h : hints) {
        x += cv.text(x, r.y + 5, h.key, style(translucent ? Color(238, 240, 242) : p.ink, true)) + 4;
        x += cv.text(x, r.y + 5, tr(h.labelKey), style(translucent ? Color(170, 177, 185) : p.dim)) + 12;
    }
}

std::vector<std::string> wrapText(const std::string& text, int maxW, int scale) {
    std::vector<std::string> lines;
    std::string cur, word;
    auto flushWord = [&] {
        if (word.empty()) return;
        std::string t = cur.empty() ? word : cur + " " + word;
        if (!cur.empty() && Canvas::textWidth(t, scale) > maxW) { lines.push_back(cur); cur = word; } else cur = t;
        word.clear();
    };
    for (char c : text) { if (c == ' ') flushWord(); else word += c; }
    flushWord();
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

void drawDescBar(Canvas& cv, const Palette& p, const std::string& name, const std::string& text, bool warn) {
    Rect r{0, kLcdH - kHintH - kDescH, kLcdW, kDescH};
    cv.fillRect(r, p.bg2);
    cv.fillRect({0, r.y, kLcdW, 1}, p.line);
    std::string full = name.empty() ? text : name + " " + text;
    auto lines = wrapText(full, kLcdW - 16);
    Color body = warn ? p.warn : p.dim;
    for (size_t i = 0; i < lines.size() && i < 3; ++i) {
        int y = r.y + 6 + int(i) * 11;
        if (i == 0 && !name.empty() && lines[0].compare(0, name.size(), name) == 0) {
            int w = cv.text(8, y, name, style(warn ? p.warn : p.ink, true));
            cv.text(8 + w + 4, y, lines[0].substr(std::min(lines[0].size(), name.size() + 1)), style(body));
        } else cv.text(8, y, lines[i], style(body));
    }
}

void drawScrollbar(Canvas& cv, const Palette& p, Rect a, int total, int visible, int top) {
    if (total <= visible) return;
    int h = std::max(12, a.h * visible / total), y = a.y + a.h * top / total;
    cv.fillRect({a.x + a.w - 3, y, 2, std::min(h, a.y + a.h - y)}, p.faint, 190);
}

void drawRow(Canvas& cv, const Palette& p, Rect r, const RowView& v) {
    int alpha = v.disabled ? 107 : 255;
    Color text = p.ink, val = p.dim;
    if (v.selected) {
        if (p.classic) { cv.fillRect(r, p.accent); text = p.accentInk; val = p.accentInk; }
        else { cv.fillRect(r, p.panel2); cv.fillRect({r.x, r.y, 2, r.h}, p.accent); val = p.accent; }
    }
    cv.fillRect({r.x, r.y + r.h - 1, r.w, 1}, v.selected && p.classic ? p.accent : p.line);
    if (v.danger && !(v.selected && p.classic)) text = p.warn;
    bool bold = v.selected && p.classic;
    cv.text(r.x + 12, r.y + 8, v.label, style(text, bold, 1, alpha));
    int right = r.x + r.w - 10;
    if (v.chevron) { cv.textRight(right, r.y + 8, "\xE2\x80\xBA", style(v.selected ? val : p.faint, false, 1, alpha)); right -= 11; }
    if (v.disabled) { drawIcon(cv, Icon::Lock, right - 11, r.y + 6, 11, p.faint, 255); right -= 15; }
    if (!v.value.empty()) cv.textRight(right, r.y + 8, v.value, style(val, bold, 1, alpha));
}

void drawDialog(Canvas& cv, const Palette& p, const DialogView& d) {
    cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0), 158);
    auto lines = wrapText(d.message, 244 - 24, 1);
    int h = 11 + 14 + 6 + int(lines.size()) * 11 + 9 + 22 + 10;
    Rect r{(kLcdW - 244) / 2, (kLcdH - h) / 2, 244, h};
    cv.fillRect(r, p.panel);
    cv.strokeRect(r, p.line);
    cv.text(r.x + 12, r.y + 11, d.title, style(d.danger ? (p.classic ? p.accent : p.warn) : p.ink, true));
    int y = r.y + 11 + 14 + 4;
    for (auto& l : lines) { cv.text(r.x + 12, y, l, style(p.dim)); y += 11; }
    int by = r.y + r.h - 10 - 22, bw = 78;
    Rect cancel{r.x + r.w - 12 - bw * 2 - 8, by, bw, 22}, ok{r.x + r.w - 12 - bw, by, bw, 22};
    auto button = [&](Rect b, const std::string& label, bool sel) {
        if (sel) cv.fillRect(b, p.accent); else cv.strokeRect(b, p.line);
        cv.textCenter(b.x + b.w / 2, b.y + 8, label, style(sel ? p.accentInk : p.dim, sel));
    };
    button(cancel, d.cancel, !d.focusOk);
    button(ok, d.ok, d.focusOk);
}

}  // namespace s650
