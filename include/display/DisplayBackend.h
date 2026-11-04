#pragma once
// Where finished frames go. Desktop: a PNG-capturing headless display and (macOS) a window.
// A real camera port would implement this against the S650's LCD path - which is UNKNOWN today.
#include <string>
#include "display/Framebuffer.h"

namespace s650 {
class DisplayBackend {
public:
    virtual ~DisplayBackend() = default;
    virtual bool initialize() = 0;
    virtual void clear() = 0;
    virtual void present(const Framebuffer& frame) = 0;   // frame is always kLcdW x kLcdH
    virtual void shutdown() {}
    virtual std::string name() const = 0;
};

// Keeps the last frame in memory; used by tests and the screenshot demo. No window.
class HeadlessDisplay : public DisplayBackend {
public:
    bool initialize() override { cleared_ = false; return true; }
    void clear() override { last_.fill(Color(0, 0, 0)); cleared_ = true; }
    void present(const Framebuffer& f) override { last_ = f; ++frames_; }
    std::string name() const override { return "headless"; }
    const Framebuffer& last() const { return last_; }
    int frames() const { return frames_; }
    bool savePng(const std::string& path, int scale = 2) const;
private:
    Framebuffer last_;
    int frames_ = 0;
    bool cleared_ = false;
};
}  // namespace s650
