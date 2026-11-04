#include "display/RawFramebufferDisplay.h"
#include <algorithm>
#include <string>

namespace s650 {

int bytesPerPixel(PixelFormat f) {
    switch (f) { case PixelFormat::RGB888: return 3; case PixelFormat::RGB565: return 2; default: return 1; }
}

uint32_t packPixel(PixelFormat f, Color c) {
    switch (f) {
        case PixelFormat::RGB888: return (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
        case PixelFormat::RGB565: return (uint32_t(c.r >> 3) << 11) | (uint32_t(c.g >> 2) << 5) | uint32_t(c.b >> 3);
        case PixelFormat::RGB332: return (uint32_t(c.r >> 5) << 5) | (uint32_t(c.g >> 5) << 2) | uint32_t(c.b >> 6);
        case PixelFormat::Gray8: return uint32_t((c.r * 77 + c.g * 150 + c.b * 29) >> 8);
        case PixelFormat::Palette8: {
            auto q = [](uint8_t v) { return (int(v) * 5 + 127) / 255; };      // 0..5
            return uint32_t(16 + 36 * q(c.r) + 6 * q(c.g) + q(c.b));
        }
    }
    return 0;
}

Color unpackPixel(PixelFormat f, uint32_t v) {
    auto e5 = [](uint32_t x) { return uint8_t((x << 3) | (x >> 2)); };
    switch (f) {
        case PixelFormat::RGB888: return Color(uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v));
        case PixelFormat::RGB565: return Color(e5((v >> 11) & 31), uint8_t((((v >> 5) & 63) << 2) | (((v >> 5) & 63) >> 4)), e5(v & 31));
        case PixelFormat::RGB332: { uint32_t r = (v >> 5) & 7, g = (v >> 2) & 7, b = v & 3;
            return Color(uint8_t(r * 255 / 7), uint8_t(g * 255 / 7), uint8_t(b * 255 / 3)); }
        case PixelFormat::Gray8: return Color(uint8_t(v), uint8_t(v), uint8_t(v));
        case PixelFormat::Palette8: { if (v < 16 || v > 231) return Color(0, 0, 0); v -= 16;
            return Color(uint8_t((v / 36) * 51), uint8_t(((v / 6) % 6) * 51), uint8_t((v % 6) * 51)); }
    }
    return Color();
}

static const char* formatName(PixelFormat f) {
    switch (f) { case PixelFormat::RGB888: return "RGB888"; case PixelFormat::RGB565: return "RGB565"; case PixelFormat::RGB332: return "RGB332";
                 case PixelFormat::Gray8: return "Gray8"; default: return "Palette8"; }
}

std::string RawFramebufferDisplay::name() const {
    return "raw-framebuffer(" + std::to_string(cfg_.width) + "x" + std::to_string(cfg_.height) + " " + formatName(cfg_.format) + ")";
}

bool RawFramebufferDisplay::initialize() {
    if (cfg_.width <= 0 || cfg_.height <= 0) return false;
    buf_.assign(size_t(cfg_.width) * size_t(cfg_.height) * size_t(bytesPerPixel(cfg_.format)), 0);
    prev_ = buf_; dirty_ = {0, 0, 0, 0}; presents_ = 0;
    return true;
}

void RawFramebufferDisplay::clear() { std::fill(buf_.begin(), buf_.end(), uint8_t(0)); }

void RawFramebufferDisplay::present(const Framebuffer& f) {
    // 1. map the logical frame onto the panel (RGB888 intermediate)
    Framebuffer panel(cfg_.width, cfg_.height);
    panel.fill(Color(0, 0, 0));
    Canvas cv(panel);
    if (cfg_.scale == ScaleMode::Stretch) cv.blit(f, {0, 0, f.width(), f.height()}, {0, 0, cfg_.width, cfg_.height});
    else cv.blit(f, {0, 0, f.width(), f.height()}, {(cfg_.width - f.width()) / 2, (cfg_.height - f.height()) / 2, f.width(), f.height()});
    // 2. convert to the raw pixel format
    const int bpp = bytesPerPixel(cfg_.format);
    prev_ = buf_;
    for (int y = 0; y < cfg_.height; ++y)
        for (int x = 0; x < cfg_.width; ++x) {
            uint32_t v = packPixel(cfg_.format, Framebuffer::unpack(panel.get(x, y)));
            uint8_t* p = &buf_[(size_t(y) * size_t(cfg_.width) + size_t(x)) * size_t(bpp)];
            if (bpp == 1) p[0] = uint8_t(v);
            else if (bpp == 2) { if (cfg_.bigEndianWords) { p[0] = uint8_t(v >> 8); p[1] = uint8_t(v); } else { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); } }
            else { p[0] = uint8_t(v >> 16); p[1] = uint8_t(v >> 8); p[2] = uint8_t(v); }
        }
    // 3. dirty rectangle versus the previous frame (what a slow display bus would need to transfer)
    int x0 = cfg_.width, y0 = cfg_.height, x1 = -1, y1 = -1;
    for (int y = 0; y < cfg_.height; ++y)
        for (int x = 0; x < cfg_.width; ++x) {
            size_t o = (size_t(y) * size_t(cfg_.width) + size_t(x)) * size_t(bpp);
            if (!std::equal(buf_.begin() + long(o), buf_.begin() + long(o) + bpp, prev_.begin() + long(o))) {
                x0 = std::min(x0, x); y0 = std::min(y0, y); x1 = std::max(x1, x); y1 = std::max(y1, y);
            }
        }
    dirty_ = x1 < 0 ? Rect{0, 0, 0, 0} : Rect{x0, y0, x1 - x0 + 1, y1 - y0 + 1};
    ++presents_;
}

Framebuffer RawFramebufferDisplay::decode() const {
    Framebuffer out(cfg_.width, cfg_.height);
    const int bpp = bytesPerPixel(cfg_.format);
    for (int y = 0; y < cfg_.height; ++y)
        for (int x = 0; x < cfg_.width; ++x) {
            const uint8_t* p = &buf_[(size_t(y) * size_t(cfg_.width) + size_t(x)) * size_t(bpp)];
            uint32_t v = bpp == 1 ? p[0] : bpp == 2 ? (cfg_.bigEndianWords ? (uint32_t(p[0]) << 8 | p[1]) : (uint32_t(p[1]) << 8 | p[0]))
                                                    : (uint32_t(p[0]) << 16 | uint32_t(p[1]) << 8 | p[2]);
            out.set(x, y, Framebuffer::pack(unpackPixel(cfg_.format, v)));
        }
    return out;
}

}  // namespace s650
