#pragma once
// macOS desktop window: shows the 320x240 framebuffer scaled up and turns key presses into camera button actions.
// Development convenience only. Uses system frameworks, no third-party code.
#include <deque>
#include <memory>
#include "display/DisplayBackend.h"
#include "input/InputBackend.h"

namespace s650 {
class CocoaWindow : public DisplayBackend, public InputBackend {
public:
    explicit CocoaWindow(int scale = 3);
    ~CocoaWindow() override;
    bool initialize() override;                 // both interfaces share this
    void clear() override;
    void present(const Framebuffer& f) override;   // also pumps window events
    void shutdown() override;
    bool poll(InputEvent& out) override;
    std::string name() const override { return "cocoa-window"; }
    bool closed() const;
private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};
}  // namespace s650
