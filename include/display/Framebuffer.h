#pragma once
// Software framebuffer and 2D drawing primitives. This is the ONLY rendering surface the interface uses, so
// anything that can show a 320x240 RGB buffer (a window, a PNG, a future camera OSD) can display it.
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "core/Types.h"

namespace s650 {

class Framebuffer {
public:
    Framebuffer(int w = kLcdW, int h = kLcdH) : w_(w), h_(h), px_(size_t(w) * size_t(h), 0xFF000000u) {}
    int width() const { return w_; }
    int height() const { return h_; }
    uint32_t* data() { return px_.data(); }
    const uint32_t* data() const { return px_.data(); }
    uint32_t get(int x, int y) const { return px_[size_t(y) * size_t(w_) + size_t(x)]; }
    void set(int x, int y, uint32_t v) { px_[size_t(y) * size_t(w_) + size_t(x)] = v; }
    void fill(Color c) { for (auto& p : px_) p = pack(c); }
    void scaleLuma(float f);                 // simulated LCD brightness (backlight stand-in)
    uint64_t hash() const;                   // FNV-1a over pixels, used by rendering tests
    static uint32_t pack(Color c) { return 0xFF000000u | (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b; }
    static Color unpack(uint32_t v) { return Color(uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)); }
private:
    int w_, h_;
    std::vector<uint32_t> px_;
};

struct TextStyle {
    int scale = 1;           // 1 = 5x7 glyph cell; 2 and 3 are integer enlargements
    bool bold = false;
    Color color{255, 255, 255};
    uint8_t alpha = 255;
    bool shadow = false;     // 1 px dark shadow used on top of the live image
};

using Pt = std::pair<float, float>;

class Canvas {
public:
    explicit Canvas(Framebuffer& fb) : fb_(fb), clip_{0, 0, fb.width(), fb.height()} {}
    Framebuffer& framebuffer() { return fb_; }

    void pushClip(Rect r);
    void popClip();

    void blendPixel(int x, int y, Color c, int alpha);
    void fillRect(Rect r, Color c, int alpha = 255);
    void strokeRect(Rect r, Color c, int thickness = 1, int alpha = 255);
    void gradientV(Rect r, Color top, Color bottom);
    void line(float x0, float y0, float x1, float y1, float thickness, Color c, int alpha = 255);
    void circle(float cx, float cy, float r, float thickness, Color c, int alpha = 255);
    void fillCircle(float cx, float cy, float r, Color c, int alpha = 255);
    void fillPoly(const std::vector<Pt>& pts, Color c, int alpha = 255);
    void polyline(const std::vector<Pt>& pts, bool closed, float thickness, Color c, int alpha = 255);

    // Scaled copy (bilinear). alpha < 255 gives a cross-fade when called twice.
    void blit(const Framebuffer& src, Rect srcRect, Rect dst, int alpha = 255);

    // Allocation-free text interface for embedded rendering.
    int text(int x, int y, const char* utf8, const TextStyle& st);
    int textRight(int xRight, int y, const char* utf8, const TextStyle& st);
    int textCenter(int xCenter, int y, const char* utf8, const TextStyle& st);
    static int textWidth(const char* utf8, int scale = 1, bool bold = false);

    // Desktop compatibility overloads.
    int text(int x, int y, const std::string& utf8, const TextStyle& st);
    int textRight(int xRight, int y, const std::string& utf8, const TextStyle& st);
    int textCenter(int xCenter, int y, const std::string& utf8, const TextStyle& st);
    static int textWidth(const std::string& utf8, int scale = 1, bool bold = false);
    static int lineHeight(int scale = 1) { return 9 * scale; }

private:
    Framebuffer& fb_;
    Rect clip_;
    std::vector<Rect> clips_;
};

}  // namespace s650
