#include "display/Icons.h"
#include <cmath>

namespace s650 {
namespace {

struct Op { char t; float a, b, c, d; };   // L line, R rect, C circle (a,b centre; c radius), P polygon edge chain uses L
using Ops = std::vector<Op>;

Ops poly(std::initializer_list<Pt> pts, bool close) {
    Ops o; Pt prev{}; bool first = true; Pt start{};
    for (auto p : pts) { if (first) { start = p; first = false; } else o.push_back({'L', prev.first, prev.second, p.first, p.second}); prev = p; }
    if (close) o.push_back({'L', prev.first, prev.second, start.first, start.second});
    return o;
}
void add(Ops& a, const Ops& b) { a.insert(a.end(), b.begin(), b.end()); }

Ops shape(Icon i) {
    Ops o;
    switch (i) {
    case Icon::Camera:
        o.push_back({'R', 3, 7.5f, 18, 11.5f}); o.push_back({'C', 12, 13.2f, 3.3f, 0});
        add(o, poly({{8.2f, 7.5f}, {9.6f, 5}, {14.4f, 5}, {15.8f, 7.5f}}, false)); break;
    case Icon::Size:
        o.push_back({'R', 4.5f, 6, 15, 12});
        add(o, poly({{7, 16}, {10.5f, 12}, {13, 14.5f}, {15, 12.5f}, {17.5f, 16}}, false)); break;
    case Icon::Focus:
        add(o, poly({{4, 9}, {4, 4}, {9, 4}}, false)); add(o, poly({{15, 4}, {20, 4}, {20, 9}}, false));
        add(o, poly({{20, 15}, {20, 20}, {15, 20}}, false)); add(o, poly({{9, 20}, {4, 20}, {4, 15}}, false));
        o.push_back({'C', 12, 12, 2.2f, 0}); break;
    case Icon::Flash: case Icon::FlashOff:
        add(o, poly({{13.2f, 3}, {6, 13.2f}, {11, 13.2f}, {9.8f, 21}, {18, 9.8f}, {12.8f, 9.8f}}, true));
        if (i == Icon::FlashOff) o.push_back({'L', 4, 4, 20, 20}); break;
    case Icon::Exposure:
        o.push_back({'R', 3.5f, 3.5f, 17, 17}); o.push_back({'L', 7.5f, 9, 13.5f, 9}); o.push_back({'L', 10.5f, 6, 10.5f, 12});
        o.push_back({'L', 13.5f, 16, 17, 16}); o.push_back({'L', 7.5f, 17, 11.5f, 13}); break;
    case Icon::WhiteBalance:
        o.push_back({'C', 12, 12, 4, 0});
        for (int k = 0; k < 8; ++k) { float a = float(k) * 0.7853982f; o.push_back({'L', 12 + 6.5f * std::cos(a), 12 + 6.5f * std::sin(a), 12 + 9 * std::cos(a), 12 + 9 * std::sin(a)}); }
        break;
    case Icon::Iso:     // drawn as text by drawIcon
        break;
    case Icon::Setup:
        o.push_back({'L', 4, 7, 13, 7}); o.push_back({'L', 19, 7, 20, 7}); o.push_back({'L', 4, 12, 7, 12}); o.push_back({'L', 13, 12, 20, 12});
        o.push_back({'L', 4, 17, 14, 17}); o.push_back({'C', 16, 7, 2.2f, 0}); o.push_back({'C', 10, 12, 2.2f, 0}); o.push_back({'C', 17, 17, 2.2f, 0}); break;
    case Icon::Timer:
        o.push_back({'C', 12, 13.5f, 7, 0}); o.push_back({'L', 12, 9.5f, 12, 13.5f}); o.push_back({'L', 12, 13.5f, 14.5f, 15});
        o.push_back({'L', 9.5f, 3, 14.5f, 3}); break;
    case Icon::Macro:
        o.push_back({'C', 12, 9.5f, 2.2f, 0}); o.push_back({'C', 12, 5.6f, 1.8f, 0}); o.push_back({'C', 15.9f, 9.5f, 1.8f, 0});
        o.push_back({'C', 12, 13.4f, 1.8f, 0}); o.push_back({'C', 8.1f, 9.5f, 1.8f, 0}); o.push_back({'L', 12, 15.5f, 12, 21}); break;
    case Icon::Card:
        add(o, poly({{6.5f, 3}, {14.5f, 3}, {18, 6.5f}, {18, 21}, {6.5f, 21}}, true)); o.push_back({'L', 14.5f, 3, 14.5f, 7}); o.push_back({'L', 14.5f, 7, 18, 7}); break;
    case Icon::Lock:
        o.push_back({'R', 6, 11, 12, 9});
        add(o, poly({{8.5f, 11}, {8.5f, 8.5f}, {10, 6.5f}, {14, 6.5f}, {15.5f, 8.5f}, {15.5f, 11}}, false)); break;
    case Icon::Images:
        o.push_back({'R', 3.5f, 5, 17, 14}); add(o, poly({{4, 16.5f}, {9, 11.5f}, {13, 15.5f}, {15.5f, 13}, {19.5f, 17}}, false));
        o.push_back({'C', 9, 9.5f, 1.4f, 0}); break;
    }
    return o;
}
}  // namespace

void drawIcon(Canvas& cv, Icon icon, int x, int y, int size, Color col, int alpha) {
    float k = float(size) / 24.f, th = std::max(1.0f, 1.6f * k);
    if (icon == Icon::Iso) {
        TextStyle st; st.color = col; st.alpha = uint8_t(alpha);
        cv.textCenter(x + size / 2, y + size / 2 - 3, "ISO", st);
        return;
    }
    for (const Op& o : shape(icon)) {
        if (o.t == 'L') cv.line(x + o.a * k, y + o.b * k, x + o.c * k, y + o.d * k, th, col, alpha);
        else if (o.t == 'C') cv.circle(x + o.a * k, y + o.b * k, o.c * k, th, col, alpha);
        else if (o.t == 'R') {
            float x0 = x + o.a * k, y0 = y + o.b * k, x1 = x0 + o.c * k, y1 = y0 + o.d * k;
            cv.polyline({{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, true, th, col, alpha);
        }
    }
}

void drawBattery(Canvas& cv, int x, int y, int w, int h, int pct, Color col) {
    cv.strokeRect({x, y, w - 3, h}, col, 1);
    cv.fillRect({x + w - 3, y + h / 2 - 2, 2, 4}, col);
    int segs = pct > 66 ? 3 : pct > 33 ? 2 : pct > 12 ? 1 : 0;
    int sw = (w - 3 - 4) / 3;
    for (int i = 0; i < segs; ++i) cv.fillRect({x + 2 + i * (sw + 1), y + 2, sw, h - 4}, col);
}

Icon categoryIcon(int c) {
    static const Icon order[8] = {Icon::Camera, Icon::Size, Icon::Focus, Icon::Flash, Icon::Exposure, Icon::WhiteBalance, Icon::Iso, Icon::Setup};
    return order[c < 0 ? 0 : c > 7 ? 7 : c];
}

}  // namespace s650
