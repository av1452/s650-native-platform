// Desktop entry point for the SIMULATED S650 interface. This program is not camera firmware and cannot run on the camera.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include "camera/CameraBackend.h"
#include "core/Application.h"
#include "core/DesktopRig.h"
#include "display/RawFramebufferDisplay.h"
#include "display/TestPattern.h"
#ifdef S650_HAVE_WINDOW
#include "display/CocoaWindow.h"
#endif

using namespace s650;

namespace {

void usage() {
    std::puts(
        "s650_native - simulated native interface for the Sony DSC-S650 concept (NOT camera firmware)\n"
        "  --window [--scale N]   open a desktop window (macOS build). Keys: arrows, Enter=OK, Esc=Back, M=Menu, P=Play,\n"
        "                         D=Disp, W/T=zoom, Space=shutter (hold=focus, release=shoot), [ ]=mode, O=power, Del, K=protect\n"
        "  --tour DIR             headless: run a scripted tour and write PNG screenshots into DIR\n"
        "  --pattern DIR          write the LCD test pattern (320x240 logical, 480x240 RGB565 raw + decoded PNG) into DIR\n"
        "  --settings FILE        persist settings in FILE (default: none)\n"
        "  --no-camera            use the UnavailableCamera backend (shows how missing hardware fails)\n"
        "  --fast                 skip the startup animation\n"
        "  --frames N             window mode: quit after N frames (for smoke tests)\n");
}

// Shows how the interface behaves when no camera backend works: no picture is invented and every camera action reports why.
int runNoCameraTour(const std::string& dir) {
    ManualClock clock; HeadlessDisplay display; ScriptedInput input; UnavailableCamera camera; SimulatedStorage storage;
    Application app(Backends{display, input, camera, storage, clock});
    if (!app.start(false)) return 1;
    std::fputs(app.startupLog().c_str(), stdout);
    app.settings().setup.startup = false;
    auto run = [&](int ms) { for (int t = 0; t < ms; t += 16) { clock.advance(16); app.tick(); } };
    auto snap = [&](const char* n) { run(32); display.savePng(dir + "/" + n + ".png", 2); std::printf("wrote %s/%s.png\n", dir.c_str(), n); };
    input.tap(Action::Power); run(100); snap("nocamera-1-shooting");
    input.push(Action::Shutter, Phase::Press); run(300); input.push(Action::Shutter, Phase::Release); run(150); snap("nocamera-2-shutter-refused");
    input.tap(Action::Menu); run(50);
    app.ui().show(ScreenId::Menu, 7); for (int i = 0; i < 9; ++i) { input.tap(Action::Down); run(32); }
    input.tap(Action::Ok); run(50); snap("nocamera-3-status");
    return 0;
}

// Renders the display test pattern through a raw 480x240 RGB565 framebuffer (a hypothetical panel format) and writes reference files.
int runPattern(const std::string& dir) {
    Framebuffer logical; Canvas cv(logical); drawTestPattern(cv);
    writePng(dir + "/lcd-test-pattern-320x240-logical.png", logical, 2);
    PanelConfig cfg; cfg.width = 480; cfg.height = 240; cfg.format = PixelFormat::RGB565; cfg.scale = ScaleMode::Stretch;
    RawFramebufferDisplay raw(cfg); raw.initialize(); raw.present(logical);
    writePng(dir + "/lcd-test-pattern-480x240-rgb565-decoded.png", raw.decode(), 2);
    std::string binPath = dir + "/lcd-test-pattern-480x240-rgb565.bin";
    if (FILE* f = std::fopen(binPath.c_str(), "wb")) { std::fwrite(raw.buffer().data(), 1, raw.buffer().size(), f); std::fclose(f); }
    std::printf("wrote %s (%zu bytes) and PNGs\n", binPath.c_str(), raw.buffer().size());
    return 0;
}

int runTour(const std::string& dir, bool noCamera, const std::string& settingsPath) {
    if (noCamera) return runNoCameraTour(dir);
    DesktopRig r(settingsPath);
    auto snap = [&](const char* name) { r.advance(32); std::string p = dir + "/" + name + ".png"; if (!r.savePng(p, 2)) std::fprintf(stderr, "cannot write %s\n", p.c_str()); else std::printf("wrote %s\n", p.c_str()); };
    if (!r.app.start(true)) { std::fputs("start failed\n", stderr); return 1; }
    r.advance(700); snap("01-startup");
    r.advance(1500); snap("02-shooting-standard");
    auto& s = r.app.settings();
    s.shoot.mode = 1; s.shoot.ev = 8; s.shoot.wb = 2; applyDensity(s, 2); s.display.histogram = true; s.display.grid = 1;
    r.advance(700); snap("03-shooting-detailed");
    s.display.histogram = false; s.display.grid = 0; applyDensity(s, 1);
    r.input.push(Action::Shutter, Phase::Press); r.advance(400); snap("04-autofocus-lock");
    r.input.push(Action::Shutter, Phase::Release); r.advance(900);
    r.tap(Action::Menu); snap("05-menu-camera");
    r.tap(Action::Ok); r.tap(Action::Down); snap("06-menu-picker");
    r.tap(Action::Back); r.tap(Action::Left); r.tap(Action::Down, 4); r.tap(Action::Right); r.tap(Action::Ok); r.tap(Action::Right, 3); snap("07-menu-slider");
    r.tap(Action::Back); r.tap(Action::Left); r.tap(Action::Down, 3); r.tap(Action::Right); snap("08-menu-setup");
    r.tap(Action::Down, 7); r.tap(Action::Ok); snap("09-display-options");
    r.tap(Action::Down, 12); r.tap(Action::Right); snap("10-appearance-light");
    r.tap(Action::Right); snap("11-appearance-classic");
    r.tap(Action::Back); snap("12-classic-menu");
    s.display.style = 0;
    r.tap(Action::Menu); r.tap(Action::Play); snap("13-playback-index");
    r.tap(Action::Right, 3); snap("14-playback-protected");
    r.tap(Action::Left, 2); r.tap(Action::Ok); r.tap(Action::Up, 2); snap("15-playback-viewer");
    r.tap(Action::Back); r.tap(Action::Delete); snap("16-delete-confirm");
    r.tap(Action::Back); r.tap(Action::Menu); snap("17-playback-menu");
    r.tap(Action::Ok); r.advance(4400); snap("18-slideshow-fade");
    r.tap(Action::Back); r.tap(Action::Play); r.input.push(Action::Power); r.advance(700); snap("19-shutdown");
    r.advance(1500); snap("20-off");
    std::puts("tour finished");
    return 0;
}

#ifdef S650_HAVE_WINDOW
int runWindow(int scale, bool fast, bool noCamera, const std::string& settingsPath, long maxFrames) {
    CocoaWindow win(scale);
    SystemClock clock;
    SimulatedCamera sim;
    UnavailableCamera none;
    CameraBackend& cam = noCamera ? static_cast<CameraBackend&>(none) : static_cast<CameraBackend&>(sim);
    SimulatedStorage storage(settingsPath);
    Application app(Backends{win, win, cam, storage, clock});
    if (!app.start(false)) { std::fputs(app.startupLog().c_str(), stderr); return 1; }
    std::fputs(app.startupLog().c_str(), stdout);
    if (fast) app.settings().setup.startup = false;
    app.system().pressPower();
    long frames = 0;
    while (!app.quitRequested() && !win.closed() && (maxFrames <= 0 || frames < maxFrames)) {
        app.tick();
        ++frames;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}
#endif

}  // namespace

int main(int argc, char** argv) {
    std::string tour, settings, pattern;
    bool window = false, fast = false, noCamera = false;
    int scale = 3; long frames = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--tour" && i + 1 < argc) tour = argv[++i];
        else if (a == "--pattern" && i + 1 < argc) pattern = argv[++i];
        else if (a == "--window") window = true;
        else if (a == "--scale" && i + 1 < argc) scale = std::atoi(argv[++i]);
        else if (a == "--settings" && i + 1 < argc) settings = argv[++i];
        else if (a == "--fast") fast = true;
        else if (a == "--no-camera") noCamera = true;
        else if (a == "--frames" && i + 1 < argc) frames = std::atol(argv[++i]);
        else { usage(); return a == "--help" ? 0 : 2; }
    }
    if (!pattern.empty()) return runPattern(pattern);
    if (!tour.empty()) return runTour(tour, noCamera, settings);
#ifdef S650_HAVE_WINDOW
    if (window || argc == 1) return runWindow(scale, fast, noCamera, settings, frames);
#else
    (void)window; (void)fast; (void)scale; (void)frames;
#endif
    usage();
    return argc == 1 ? 0 : 2;
}
