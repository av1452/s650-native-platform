#pragma once
// A DisplayBackend that converts the 320x240 logical frame into a RAW fixed-format framebuffer (width x height x pixel format).
// It models what a hardware LCD/OSD backend would have to do, so the conversion, scaling and dirty-rectangle logic can be written
// and tested on the desktop BEFORE any hardware facts are known. It does NOT drive any hardware, and the formats it offers are
// candidates (the real S650 pixel format is unknown); nothing here is evidence about the camera.
#include <stdint.h>
#include <vector>
#include "display/DisplayBackend.h"

namespace s650 {

enum class PixelFormat { RGB888, RGB565, RGB332, Gray8, Palette8 };
enum class ScaleMode { Stretch, Center };      // Stretch: logical frame fills the panel (non-square pixels if the aspect differs); Center: letterbox

struct PanelConfig {
    int width = 320, height = 240;
    PixelFormat format = PixelFormat::RGB565;
    ScaleMode scale = ScaleMode::Stretch;
    bool bigEndianWords = false;               // byte order of 16-bit pixels in memory
};

int bytesPerPixel(PixelFormat f);
// Pixel conversion helpers (also used by tests). Palette8 is a fixed 6x6x6 colour cube at indices 16..231 (illustrative).
uint32_t packPixel(PixelFormat f, Color c);
Color unpackPixel(PixelFormat f, uint32_t v);

class RawFramebufferDisplay : public DisplayBackend {
public:
    explicit RawFramebufferDisplay(PanelConfig cfg) : cfg_(cfg) {}
    bool initialize() override;
    void clear() override;
    void present(const Framebuffer& f) override;
    std::string name() const override;

    const PanelConfig& config() const { return cfg_; }
    const std::vector<uint8_t>& buffer() const { return buf_; }
    size_t stride() const { return size_t(cfg_.width) * size_t(bytesPerPixel(cfg_.format)); }
    size_t bytesPerFrame() const { return buf_.size(); }
    Framebuffer decode() const;                // raw buffer back to RGB (for inspection / PNG)
    // Smallest rectangle (in PANEL pixels) that changed in the last present(); {0,0,0,0} when nothing changed.
    Rect lastDirty() const { return dirty_; }
    int presents() const { return presents_; }

private:
    PanelConfig cfg_;
    std::vector<uint8_t> buf_, prev_;
    Rect dirty_{0, 0, 0, 0};
    int presents_ = 0;
};

}  // namespace s650
