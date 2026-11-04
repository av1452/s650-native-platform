#include <cstdio>
#include <fstream>
#include <iterator>
#include "TestFramework.h"
#include "camera/Exposure.h"
#include "core/DesktopRig.h"
#include "ui/Screens.h"

using namespace s650;

TEST(canvas_fill_alpha_and_clip) {
    Framebuffer fb(8, 8); fb.fill(Color(0, 0, 0));
    Canvas cv(fb);
    cv.fillRect({0, 0, 4, 4}, Color(255, 255, 255), 128);
    uint32_t p = fb.get(1, 1);
    CHECK(((p >> 16) & 255) >= 127 && ((p >> 16) & 255) <= 129);
    cv.pushClip({6, 6, 2, 2});
    cv.fillRect({0, 0, 8, 8}, Color(255, 0, 0));
    cv.popClip();
    CHECK_EQ(fb.get(1, 1) & 0xFF0000, p & 0xFF0000);            // untouched outside the clip
    CHECK_EQ(Framebuffer::unpack(fb.get(7, 7)).r, 255);
}

TEST(canvas_line_and_polygon_cover_pixels) {
    Framebuffer fb(20, 20); fb.fill(Color(0, 0, 0));
    Canvas cv(fb);
    cv.line(2, 10, 18, 10, 2.f, Color(255, 255, 255));
    CHECK(Framebuffer::unpack(fb.get(10, 10)).r > 200);
    CHECK(Framebuffer::unpack(fb.get(10, 3)).r == 0);
    cv.fillPoly({{2, 14}, {18, 14}, {10, 19}}, Color(0, 255, 0));
    CHECK(Framebuffer::unpack(fb.get(10, 16)).g == 255);
}

TEST(font_metrics) {
    CHECK(Canvas::textWidth("Hello") > 20);
    CHECK_EQ(Canvas::textWidth("11"), Canvas::textWidth("88"));   // tabular digits
    CHECK_EQ(Canvas::textWidth("Hi", 1, true), Canvas::textWidth("Hi", 1, false) + 1);
    CHECK_EQ(Canvas::textWidth("Hi", 2), Canvas::textWidth("Hi", 1) * 2);
    CHECK_EQ(Canvas::textWidth(""), 0);
    CHECK(Canvas::textWidth("\xE2\x80\xBA") > 0);                 // symbol glyph exists
    CHECK(Canvas::textWidth("\xC3\xA9") > 0);                     // unknown glyph falls back instead of crashing
}

TEST(png_writer_produces_valid_signature) {
    Framebuffer fb; fb.fill(Color(10, 20, 30));
    const char* path = "/tmp/s650_test.png";
    CHECK(writePng(path, fb, 1));
    std::ifstream f(path, std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(data.size() > 8);
    CHECK_EQ(data.substr(1, 3), std::string("PNG"));
    CHECK(data.find("IHDR") != std::string::npos);
    CHECK(data.find("IDAT") != std::string::npos);
    CHECK(data.find("IEND") != std::string::npos);
    CHECK(!writePng("/nonexistent_dir_s650/x.png", fb, 1));
    std::remove(path);
}

static uint64_t menuHash(int style) {
    DesktopRig r; r.startReady();
    r.app.settings().display.style = style;
    r.tap(Action::Menu);
    r.advance(100);
    return r.display.last().hash();
}

TEST(render_is_deterministic_and_themes_differ) {
    CHECK_EQ(menuHash(0), menuHash(0));
    CHECK(menuHash(0) != menuHash(1));
    CHECK(menuHash(1) != menuHash(2));
    CHECK(menuHash(0) != menuHash(2));
}

TEST(render_selected_row_uses_theme_accent) {
    {
        DesktopRig r; r.startReady();
        r.tap(Action::Menu); r.advance(100);
        const Framebuffer& f = r.display.last();
        Color c = Framebuffer::unpack(f.get(38, 30));              // 2 px accent bar at the left of the selected row
        CHECK(c.r == 0xf2 && c.g == 0x8a && c.b == 0x1b);
    }
    {
        DesktopRig r; r.startReady();
        r.app.settings().display.style = 2;
        r.tap(Action::Menu); r.advance(100);
        Color c = Framebuffer::unpack(r.display.last().get(200, 52 + 10));   // classic: whole row solid accent
        CHECK(c.r == 0xff && c.g == 0x9f && c.b == 0x1a);
    }
}

TEST(render_dialog_dims_the_screen) {
    DesktopRig r; r.startReady();
    r.tap(Action::Play); r.advance(100);
    uint32_t before = r.display.last().get(5, 100);
    r.tap(Action::Delete); r.advance(100);
    uint32_t after = r.display.last().get(5, 100);
    CHECK(((after >> 8) & 255) < ((before >> 8) & 255));
}

TEST(render_brightness_setting_scales_luma) {
    DesktopRig r; r.startReady();
    r.app.settings().setup.brightness = 3; r.tap(Action::Play); r.advance(100);
    int mid = Framebuffer::unpack(r.display.last().get(160, 215)).r;
    r.app.settings().setup.brightness = 1; r.advance(100);
    int dark = Framebuffer::unpack(r.display.last().get(160, 215)).r;
    r.app.settings().setup.brightness = 5; r.advance(100);
    int bright = Framebuffer::unpack(r.display.last().get(160, 215)).r;
    CHECK(dark <= mid); CHECK(mid <= bright);
}

TEST(render_every_screen_in_every_theme_without_crashing) {
    for (int style = 0; style < 3; ++style) {
        DesktopRig r; r.startReady();
        r.app.settings().display.style = style;
        r.app.settings().display.histogram = true; r.app.settings().display.grid = 2;
        applyDensity(r.app.settings(), 2);
        r.advance(200);
        uint64_t h0 = r.display.last().hash();
        for (int c = 0; c < 8; ++c) { r.app.ui().show(ScreenId::Menu, c); r.advance(50); }
        r.app.ui().show(ScreenId::Menu, 7);
        for (int i = 0; i < 12; ++i) { r.tap(Action::Down); r.tap(Action::Ok); r.tap(Action::Back); }
        r.app.ui().show(ScreenId::Display, 0);
        for (int i = 0; i < 13; ++i) { r.tap(Action::Down); r.advance(20); }
        r.app.ui().show(ScreenId::Playback, 0);
        r.tap(Action::Ok); r.tap(Action::Up); r.tap(Action::Up); r.tap(Action::Tele); r.advance(30);
        r.tap(Action::Menu); r.advance(30);
        CHECK(r.display.last().hash() != h0);
        CHECK(r.display.frames() > 50);
    }
}

TEST(render_without_camera_shows_notice_not_picture) {
    ManualClock clock; HeadlessDisplay disp; ScriptedInput in; UnavailableCamera cam; SimulatedStorage st;
    Application app(Backends{disp, in, cam, st, clock});
    app.start(false); app.settings().setup.startup = false; in.tap(Action::Power);
    for (int i = 0; i < 8; ++i) { clock.advance(16); app.tick(); }
    // centre of the live view area is the dark placeholder, not scene colours
    Color c = Framebuffer::unpack(disp.last().get(40, 150));
    CHECK(c.r < 40 && c.g < 40 && c.b < 40);
}

TEST(histogram_is_normalised) {
    Framebuffer fb(40, 40); fb.fill(Color(128, 128, 128));
    auto h = histogram(fb, 32);
    CHECK_EQ(h.size(), size_t(32));
    float mx = 0; for (float v : h) { CHECK(v >= 0.f && v <= 1.f); if (v > mx) mx = v; }
    CHECK(mx == 1.f);
}

// Regression: the first screenshot tour showed black bars at the right and bottom of the live view because scenes
// drawn for 320x240 were placed in a larger buffer. The preview must cover the whole picture area.
TEST(render_live_view_covers_whole_picture_area) {
    DesktopRig r; r.startReady();
    r.advance(200);
    const Framebuffer& f = r.display.last();
    auto dark = [&](int x, int y) { Color c = Framebuffer::unpack(f.get(x, y)); return c.r < 20 && c.g < 20 && c.b < 20; };
    CHECK(!dark(316, 100));        // right edge, between the top and bottom bars
    CHECK(!dark(160, 214));        // just above the bottom bar
    CHECK(!dark(3, 120));          // left edge
}
