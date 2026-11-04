// macOS window backend (development convenience). Pumps events from present(), so no separate run loop is needed.
#import <Cocoa/Cocoa.h>
#include "display/CocoaWindow.h"
#include <set>

@interface S650View : NSView
@property(nonatomic) CGImageRef image;
@property(nonatomic) std::deque<s650::InputEvent>* queue;
@property(nonatomic) std::set<unsigned short>* down;
@end

static s650::Action mapKey(NSEvent* e) {
    switch (e.keyCode) {
        case 126: return s650::Action::Up; case 125: return s650::Action::Down; case 123: return s650::Action::Left; case 124: return s650::Action::Right;
        case 36: case 76: return s650::Action::Ok; case 53: return s650::Action::Back; case 51: case 117: return s650::Action::Delete;
        default: break;
    }
    NSString* c = e.charactersIgnoringModifiers;
    if (c.length == 0) return s650::Action::None;
    return s650::actionForKey(0, char([c characterAtIndex:0] & 0x7F));
}

@implementation S650View
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent*)e {
    s650::Action a = mapKey(e);
    if (a == s650::Action::None) return;
    bool repeatable = a == s650::Action::Up || a == s650::Action::Down || a == s650::Action::Left || a == s650::Action::Right ||
                      a == s650::Action::Wide || a == s650::Action::Tele;
    if (e.isARepeat && !repeatable) return;
    self.queue->push_back({a, s650::Phase::Press});
}
- (void)keyUp:(NSEvent*)e {
    if (mapKey(e) == s650::Action::Shutter) self.queue->push_back({s650::Action::Shutter, s650::Phase::Release});
}
- (void)drawRect:(NSRect)r {
    if (!self.image) return;
    CGContextRef g = [[NSGraphicsContext currentContext] CGContext];
    CGContextSetInterpolationQuality(g, kCGInterpolationNone);     // crisp pixels
    CGContextDrawImage(g, self.bounds, self.image);
}
@end

struct s650::CocoaWindow::Impl {
    int scale;
    NSWindow* window = nil;
    S650View* view = nil;
    std::deque<s650::InputEvent> queue;
    std::set<unsigned short> down;
    bool closed = false;
    CGImageRef image = nullptr;
};

namespace s650 {

CocoaWindow::CocoaWindow(int scale) : d_(new Impl) { d_->scale = scale < 1 ? 1 : scale; }
CocoaWindow::~CocoaWindow() { shutdown(); }

bool CocoaWindow::initialize() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSRect r = NSMakeRect(200, 200, kLcdW * d_->scale, kLcdH * d_->scale);
        d_->window = [[NSWindow alloc] initWithContentRect:r
                                                 styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                                                   backing:NSBackingStoreBuffered defer:NO];
        d_->window.title = @"S650 native interface (SIMULATION - not camera firmware)";
        d_->view = [[S650View alloc] initWithFrame:r];
        d_->view.queue = &d_->queue;
        d_->view.down = &d_->down;
        d_->window.contentView = d_->view;
        [d_->window makeFirstResponder:d_->view];
        [d_->window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
    return d_->window != nil;
}

void CocoaWindow::clear() {}

void CocoaWindow::present(const Framebuffer& f) {
    @autoreleasepool {
        std::vector<uint8_t> rgba(size_t(f.width()) * size_t(f.height()) * 4);
        for (int i = 0; i < f.width() * f.height(); ++i) {
            uint32_t p = f.data()[i];
            rgba[size_t(i) * 4] = uint8_t(p >> 16); rgba[size_t(i) * 4 + 1] = uint8_t(p >> 8); rgba[size_t(i) * 4 + 2] = uint8_t(p); rgba[size_t(i) * 4 + 3] = 255;
        }
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGDataProviderRef prov = CGDataProviderCreateWithData(nullptr, rgba.data(), rgba.size(), nullptr);
        CGImageRef img = CGImageCreate(size_t(f.width()), size_t(f.height()), 8, 32, size_t(f.width()) * 4, cs,
                                       kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast, prov, nullptr, false, kCGRenderingIntentDefault);
        // the provider data dies with this function, so force a copy into the image before returning
        CFDataRef copy = CGDataProviderCopyData(prov);
        CGDataProviderRef prov2 = CGDataProviderCreateWithCFData(copy);
        CGImageRef img2 = CGImageCreate(size_t(f.width()), size_t(f.height()), 8, 32, size_t(f.width()) * 4, cs,
                                        kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast, prov2, nullptr, false, kCGRenderingIntentDefault);
        CGImageRelease(img); CGDataProviderRelease(prov); CGDataProviderRelease(prov2); CFRelease(copy); CGColorSpaceRelease(cs);
        if (d_->image) CGImageRelease(d_->image);
        d_->image = img2;
        d_->view.image = d_->image;
        [d_->view setNeedsDisplay:YES];
        [d_->view displayIfNeeded];
        for (;;) {
            NSEvent* e = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
            if (!e) break;
            [NSApp sendEvent:e];
        }
        if (!d_->window.visible && !d_->window.miniaturized) d_->closed = true;
    }
}

void CocoaWindow::shutdown() {
    if (d_->image) { CGImageRelease(d_->image); d_->image = nullptr; }
    if (d_->window) { [d_->window close]; d_->window = nil; }
}

bool CocoaWindow::poll(InputEvent& out) {
    if (d_->queue.empty()) return false;
    out = d_->queue.front(); d_->queue.pop_front(); return true;
}

bool CocoaWindow::closed() const { return d_->closed; }

}  // namespace s650
