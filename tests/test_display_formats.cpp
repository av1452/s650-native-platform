#include "TestFramework.h"
#include "core/DesktopRig.h"
#include "display/RawFramebufferDisplay.h"
#include "display/TestPattern.h"

using namespace s650;

static int maxDiff(Color a, Color b) {
    auto d = [](int x, int y) { return x > y ? x - y : y - x; };
    int m = d(a.r, b.r); if (d(a.g, b.g) > m) m = d(a.g, b.g); if (d(a.b, b.b) > m) m = d(a.b, b.b); return m;
}

TEST(pixelformat_rgb565_known_values_and_roundtrip) {
    CHECK_EQ(packPixel(PixelFormat::RGB565, Color(255, 255, 255)), 0xFFFFu);
    CHECK_EQ(packPixel(PixelFormat::RGB565, Color(255, 0, 0)), 0xF800u);
    CHECK_EQ(packPixel(PixelFormat::RGB565, Color(0, 255, 0)), 0x07E0u);
    CHECK_EQ(packPixel(PixelFormat::RGB565, Color(0, 0, 255)), 0x001Fu);
    CHECK_EQ(packPixel(PixelFormat::RGB565, Color(0, 0, 0)), 0u);
    for (int r = 0; r < 256; r += 17) for (int g = 0; g < 256; g += 17) for (int b = 0; b < 256; b += 17) {
        Color c{uint8_t(r), uint8_t(g), uint8_t(b)};
        CHECK(maxDiff(c, unpackPixel(PixelFormat::RGB565, packPixel(PixelFormat::RGB565, c))) <= 8);
    }
}

TEST(pixelformat_other_formats_bounded_error) {
    for (int r = 0; r < 256; r += 51) for (int g = 0; g < 256; g += 51) for (int b = 0; b < 256; b += 51) {
        Color c{uint8_t(r), uint8_t(g), uint8_t(b)};
        CHECK(maxDiff(c, unpackPixel(PixelFormat::RGB332, packPixel(PixelFormat::RGB332, c))) <= 64);
        CHECK(maxDiff(c, unpackPixel(PixelFormat::Palette8, packPixel(PixelFormat::Palette8, c))) <= 26);   // 6-level cube: exact on multiples of 51
        CHECK(maxDiff(c, unpackPixel(PixelFormat::RGB888, packPixel(PixelFormat::RGB888, c))) == 0);
    }
    uint32_t idx = packPixel(PixelFormat::Palette8, Color(255, 255, 255));
    CHECK(idx >= 16 && idx <= 231);
    Color g = unpackPixel(PixelFormat::Gray8, packPixel(PixelFormat::Gray8, Color(255, 255, 255)));
    CHECK(g.r >= 253);
    CHECK_EQ(bytesPerPixel(PixelFormat::RGB565), 2); CHECK_EQ(bytesPerPixel(PixelFormat::RGB888), 3); CHECK_EQ(bytesPerPixel(PixelFormat::Palette8), 1);
}

TEST(rawfb_stretch_to_480x240_has_expected_geometry) {
    PanelConfig cfg; cfg.width = 480; cfg.height = 240; cfg.format = PixelFormat::RGB565; cfg.scale = ScaleMode::Stretch;
    RawFramebufferDisplay d(cfg); CHECK(d.initialize());
    CHECK_EQ(d.bytesPerFrame(), size_t(480 * 240 * 2)); CHECK_EQ(d.stride(), size_t(960));
    Framebuffer f; f.fill(Color(0, 0, 0)); Canvas cv(f);
    cv.fillRect({0, 0, 320, 240}, Color(255, 255, 255));
    cv.fillRect({0, 0, 10, 240}, Color(255, 0, 0));          // left strip: 10 logical px -> about 15 panel px
    d.present(f);
    Framebuffer out = d.decode();
    CHECK(Framebuffer::unpack(out.get(0, 100)).r == 255 && Framebuffer::unpack(out.get(0, 100)).g < 20);
    CHECK(Framebuffer::unpack(out.get(5, 100)).g < 40);            // still red at panel x=5
    CHECK(Framebuffer::unpack(out.get(40, 100)).g > 200);          // white well past the strip
    CHECK(Framebuffer::unpack(out.get(479, 239)).g > 200);          // far corner covered: no black margin in stretch mode
    int redEnd = 0; while (redEnd < 480 && Framebuffer::unpack(out.get(redEnd, 100)).g < 128) ++redEnd;
    CHECK(redEnd >= 14 && redEnd <= 16);                            // 10 * 480/320 = 15
}

TEST(rawfb_center_mode_letterboxes) {
    PanelConfig cfg; cfg.width = 480; cfg.height = 240; cfg.scale = ScaleMode::Center;
    RawFramebufferDisplay d(cfg); d.initialize();
    Framebuffer f; f.fill(Color(255, 255, 255));
    d.present(f);
    Framebuffer out = d.decode();
    CHECK(Framebuffer::unpack(out.get(10, 100)).g < 10);            // left bar black
    CHECK(Framebuffer::unpack(out.get(240, 100)).g > 240);          // centre white
    CHECK(Framebuffer::unpack(out.get(470, 100)).g < 10);           // right bar black
}

TEST(rawfb_word_order_and_dirty_rectangle) {
    PanelConfig cfg; cfg.width = 64; cfg.height = 48; cfg.format = PixelFormat::RGB565; cfg.scale = ScaleMode::Stretch; cfg.bigEndianWords = false;
    RawFramebufferDisplay le(cfg); le.initialize();
    cfg.bigEndianWords = true; RawFramebufferDisplay be(cfg); be.initialize();
    Framebuffer f; f.fill(Color(255, 0, 0));                         // 0xF800
    le.present(f); be.present(f);
    CHECK_EQ(int(le.buffer()[0]), 0x00); CHECK_EQ(int(le.buffer()[1]), 0xF8);
    CHECK_EQ(int(be.buffer()[0]), 0xF8); CHECK_EQ(int(be.buffer()[1]), 0x00);
    // dirty rectangle: identical frame -> empty; a small change -> a small box
    le.present(f); CHECK_EQ(le.lastDirty().w, 0);
    Canvas(f).fillRect({100, 100, 20, 10}, Color(0, 0, 255));
    le.present(f);
    Rect d = le.lastDirty();
    CHECK(d.w > 0 && d.h > 0 && d.w < 20 && d.h < 15);
    CHECK(d.x >= 18 && d.x <= 22 && d.y >= 18 && d.y <= 22);       // 100/320*64 = 20, 100/240*48 = 20
}

TEST(rawfb_full_application_runs_unchanged_on_a_fixed_framebuffer) {
    // The whole interface drives a 480x240 RGB565 framebuffer through the same DisplayBackend interface: no application change.
    ManualClock clock; RawFramebufferDisplay disp([] { PanelConfig c; c.width = 480; c.height = 240; c.format = PixelFormat::RGB565; return c; }());
    ScriptedInput in; SimulatedCamera cam; SimulatedStorage st;
    Application app(Backends{disp, in, cam, st, clock});
    CHECK(app.start(false));
    app.settings().setup.startup = false;
    in.tap(Action::Power);
    for (int i = 0; i < 10; ++i) { clock.advance(16); app.tick(); }
    in.tap(Action::Menu);
    for (int i = 0; i < 10; ++i) { clock.advance(16); app.tick(); }
    Framebuffer out = disp.decode();
    Color hdr = Framebuffer::unpack(out.get(200, 3));                // header background (modern theme #08090a)
    CHECK(hdr.r < 24 && hdr.g < 24 && hdr.b < 24);
    Color sel = Framebuffer::unpack(out.get(57, 40));                // accent bar of the selected row (about x = 38 * 1.5)
    CHECK(sel.r > 200 && sel.g > 100 && sel.b < 60);
    CHECK(disp.presents() > 15);
}

TEST(test_pattern_is_deterministic_and_has_specified_features) {
    Framebuffer a, b; Canvas ca(a), cb(b); drawTestPattern(ca); drawTestPattern(cb);
    CHECK_EQ(a.hash(), b.hash());
    CHECK(Framebuffer::unpack(a.get(20, 50)).r == 255 && Framebuffer::unpack(a.get(20, 50)).b == 255);      // white bar
    CHECK(Framebuffer::unpack(a.get(260, 50)).b == 255 && Framebuffer::unpack(a.get(260, 50)).r == 0);      // blue bar (index 6)
    CHECK(Framebuffer::unpack(a.get(5, 5)).r == 255 && Framebuffer::unpack(a.get(5, 5)).g == 0);            // red corner
    CHECK(Framebuffer::unpack(a.get(315, 235)).r == 255 && Framebuffer::unpack(a.get(315, 235)).g == 255);  // yellow corner
    CHECK_EQ(int(Framebuffer::unpack(a.get(30, 130)).r), 17);                                               // ramp step 1
    // on a 480x240 stretched panel the circle becomes an ellipse 1.5x wider than tall
    PanelConfig cfg; cfg.width = 480; cfg.height = 240; RawFramebufferDisplay d(cfg); d.initialize(); d.present(a);
    Framebuffer p = d.decode();
    int minx = 480, maxx = -1, miny = 240, maxy = -1;
    for (int y = 152; y < 232; ++y) for (int x = 20; x < 200; ++x) if (Framebuffer::unpack(p.get(x, y)).g > 200) {
        minx = x < minx ? x : minx; maxx = x > maxx ? x : maxx; miny = y < miny ? y : miny; maxy = y > maxy ? y : maxy; }
    int w = maxx - minx + 1, h = maxy - miny + 1;
    CHECK(w > 0 && h > 0);
    CHECK(double(w) / double(h) > 1.35 && double(w) / double(h) < 1.65);
}
