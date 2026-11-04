#include "TestFramework.h"
#include "core/DesktopRig.h"
#include "ui/Screens.h"

using namespace s650;

static ShootingScreen& sh(DesktopRig& r) { return static_cast<ShootingScreen&>(r.app.ui().screen(ScreenId::Shooting)); }

TEST(shooting_half_press_focuses_then_locks) {
    DesktopRig r; CHECK(r.startReady());
    CHECK(sh(r).focus() == FocusState::Idle);
    r.input.push(Action::Shutter, Phase::Press); r.advance(48);
    CHECK(sh(r).focus() == FocusState::Seeking);
    r.advance(300);
    CHECK(sh(r).focus() == FocusState::Locked);
    r.input.push(Action::Shutter, Phase::Release); r.advance(48);
}

TEST(shooting_release_captures_a_simulated_photo) {
    DesktopRig r; CHECK(r.startReady());
    size_t n = r.storage.images().size();
    r.input.push(Action::Shutter, Phase::Press); r.advance(300);
    r.input.push(Action::Shutter, Phase::Release); r.advance(100);
    CHECK_EQ(r.storage.images().size(), n + 1);
    CHECK(r.storage.images().back().pixels != nullptr);
    CHECK(sh(r).toastText().find("Saved") != std::string::npos);
}

TEST(shooting_zoom_and_mode_controls) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Tele, 30); CHECK_EQ(r.app.settings().shoot.zoom, 25);
    r.tap(Action::Wide, 40); CHECK_EQ(r.app.settings().shoot.zoom, 0);
    r.tap(Action::ModeNext); CHECK_EQ(r.app.settings().shoot.mode, 1);
    r.tap(Action::ModePrev, 2); CHECK_EQ(r.app.settings().shoot.mode, 3);   // wraps
}

TEST(shooting_shortcuts_cycle_settings) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Down); CHECK_EQ(r.app.settings().shoot.timer, 1);
    r.tap(Action::Down, 2); CHECK_EQ(r.app.settings().shoot.timer, 0);
    r.tap(Action::Right); CHECK_EQ(r.app.settings().shoot.flash, 1);
    r.tap(Action::Left); CHECK(r.app.settings().shoot.macro);
    r.app.settings().shoot.mode = 3;
    int f = r.app.settings().shoot.flash;
    r.tap(Action::Right); CHECK_EQ(r.app.settings().shoot.flash, f);          // no flash change in movie mode
}

TEST(shooting_display_cycle_order) {
    DesktopRig r; CHECK(r.startReady());
    CHECK_EQ(r.app.settings().display.density, 1);
    r.tap(Action::Up); CHECK_EQ(r.app.settings().display.density, 2);
    r.tap(Action::Disp); CHECK_EQ(r.app.settings().display.density, 0);
    r.tap(Action::Up); CHECK(!r.app.settings().display.showInfo);
    r.tap(Action::Up); CHECK(r.app.settings().display.showInfo); CHECK_EQ(r.app.settings().display.density, 1);
}

TEST(shooting_self_timer_counts_down_and_captures) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.timer = 1;                                         // 2 s
    size_t n = r.storage.images().size();
    r.input.push(Action::Shutter, Phase::Press); r.advance(300);
    r.input.push(Action::Shutter, Phase::Release); r.advance(48);
    CHECK_EQ(sh(r).countdownSeconds(), 2);
    r.advance(1000);
    CHECK_EQ(r.storage.images().size(), n);
    r.advance(1100);
    CHECK_EQ(r.storage.images().size(), n + 1);
    CHECK_EQ(sh(r).countdownSeconds(), -1);
}

TEST(shooting_self_timer_can_be_cancelled) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.timer = 2;                                         // 10 s
    size_t n = r.storage.images().size();
    r.input.push(Action::Shutter, Phase::Press); r.advance(300);
    r.input.push(Action::Shutter, Phase::Release); r.advance(100);
    CHECK(sh(r).countdownSeconds() > 0);
    r.tap(Action::Back);
    CHECK_EQ(sh(r).countdownSeconds(), -1);
    r.advance(11000);
    CHECK_EQ(r.storage.images().size(), n);
}

TEST(shooting_movie_record_start_stop) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.mode = 3;
    r.input.push(Action::Shutter, Phase::Press); r.advance(300);
    r.input.push(Action::Shutter, Phase::Release); r.advance(100);
    CHECK(sh(r).recording());
    CHECK(r.camera.recording());
    r.input.push(Action::Shutter, Phase::Press); r.advance(100);
    r.input.push(Action::Shutter, Phase::Release); r.advance(100);
    CHECK(!sh(r).recording());
    CHECK(!r.camera.recording());
}

TEST(shooting_leaving_movie_mode_stops_recording) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.mode = 3;
    r.input.push(Action::Shutter, Phase::Press); r.advance(300);
    r.input.push(Action::Shutter, Phase::Release); r.advance(100);
    CHECK(sh(r).recording());
    r.tap(Action::ModeNext);
    CHECK(!sh(r).recording());
}

TEST(shooting_memory_full_is_reported) {
    ManualClock clock; HeadlessDisplay disp; ScriptedInput in; SimulatedCamera cam;
    SimulatedStorage st("", true, 30000000);                                   // 16 samples already exceed this
    Application app(Backends{disp, in, cam, st, clock});
    CHECK(app.start(false));
    app.settings().setup.startup = false;
    in.tap(Action::Power);
    for (int i = 0; i < 6; ++i) { clock.advance(16); app.tick(); }
    CHECK_EQ(app.ui().remainingShots(), 0);
    size_t n = st.images().size();
    in.push(Action::Shutter, Phase::Press); for (int i = 0; i < 20; ++i) { clock.advance(16); app.tick(); }
    in.push(Action::Shutter, Phase::Release); for (int i = 0; i < 10; ++i) { clock.advance(16); app.tick(); }
    CHECK_EQ(st.images().size(), n);
    CHECK(app.ui().lastAnnouncement().find("full") != std::string::npos);
}

TEST(shooting_without_camera_fails_visibly) {
    ManualClock clock; HeadlessDisplay disp; ScriptedInput in; UnavailableCamera cam; SimulatedStorage st;
    Application app(Backends{disp, in, cam, st, clock});
    CHECK(app.start(false));                                                   // missing camera is not fatal
    CHECK(app.startupLog().find("continuing without camera") != std::string::npos);
    app.settings().setup.startup = false;
    in.tap(Action::Power);
    for (int i = 0; i < 6; ++i) { clock.advance(16); app.tick(); }
    size_t n = st.images().size();
    in.push(Action::Shutter, Phase::Press); for (int i = 0; i < 20; ++i) { clock.advance(16); app.tick(); }
    in.push(Action::Shutter, Phase::Release); for (int i = 0; i < 10; ++i) { clock.advance(16); app.tick(); }
    CHECK_EQ(st.images().size(), n);                                           // no photo invented
    CHECK(app.ui().lastAnnouncement().find("unavailable") != std::string::npos);
    CHECK(app.ui().remainingShots() > 0);                                      // UI still functions
}

TEST(shooting_redeye_delays_the_shot_for_pre_flash) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.flash = 1; r.app.settings().shoot.redeye = true;    // flash forced on
    size_t n = r.storage.images().size();
    r.input.push(Action::Shutter, Phase::Press); r.advance(300);
    r.input.push(Action::Shutter, Phase::Release); r.advance(100);
    CHECK_EQ(r.storage.images().size(), n);                                    // pre-flashes still running
    r.advance(500);
    CHECK_EQ(r.storage.images().size(), n + 1);
}

TEST(shooting_without_camera_never_claims_focus) {
    ManualClock clock; HeadlessDisplay disp; ScriptedInput in; UnavailableCamera cam; SimulatedStorage st;
    Application app(Backends{disp, in, cam, st, clock});
    CHECK(app.start(false));
    app.settings().setup.startup = false;
    in.tap(Action::Power);
    for (int i = 0; i < 6; ++i) { clock.advance(16); app.tick(); }
    in.push(Action::Shutter, Phase::Press);
    for (int i = 0; i < 40; ++i) { clock.advance(16); app.tick(); }
    auto& sh = static_cast<ShootingScreen&>(app.ui().screen(ScreenId::Shooting));
    CHECK(sh.focus() == FocusState::Idle);
    in.push(Action::Shutter, Phase::Release);
    for (int i = 0; i < 10; ++i) { clock.advance(16); app.tick(); }
}
