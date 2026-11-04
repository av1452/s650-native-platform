#include "TestFramework.h"
#include "core/DesktopRig.h"
#include "ui/Screens.h"

using namespace s650;

TEST(system_starts_off_and_power_boots_with_animation) {
    DesktopRig r;
    CHECK(r.app.start(false));
    CHECK(r.app.system().state() == PowerState::Off);
    r.input.tap(Action::Power); r.advance(50);
    CHECK(r.app.system().state() == PowerState::Booting);
    r.advance(1700);
    CHECK(r.app.system().state() == PowerState::Booting);
    r.advance(300);
    CHECK(r.app.system().state() == PowerState::On);
    CHECK(r.app.ui().current() == ScreenId::Shooting);
}

TEST(system_any_key_skips_the_startup_animation) {
    DesktopRig r;
    CHECK(r.app.start(true));
    r.advance(200);
    CHECK(r.app.system().state() == PowerState::Booting);
    r.input.tap(Action::Down); r.advance(50);
    CHECK(r.app.system().state() == PowerState::On);
}

TEST(system_startup_animation_can_be_disabled) {
    DesktopRig r;
    CHECK(r.app.start(false));
    r.app.settings().setup.startup = false;
    r.input.tap(Action::Power); r.advance(50);
    CHECK(r.app.system().state() == PowerState::On);
}

TEST(system_shutdown_sequence_ends_off) {
    DesktopRig r; CHECK(r.startReady());
    r.input.tap(Action::Power); r.advance(60);
    CHECK(r.app.system().state() == PowerState::ShuttingDown);
    r.input.tap(Action::Menu); r.advance(32);                        // input ignored while shutting down
    CHECK(r.app.ui().current() == ScreenId::Shooting);
    r.advance(1400);
    CHECK(r.app.system().state() == PowerState::Off);
    CHECK(r.app.ui().current() == ScreenId::Off);
}

TEST(system_off_ignores_buttons_except_power_and_ok) {
    DesktopRig r; CHECK(r.app.start(false));
    r.input.tap(Action::Menu); r.input.tap(Action::Play); r.input.tap(Action::Shutter); r.advance(100);
    CHECK(r.app.system().state() == PowerState::Off);
    r.input.tap(Action::Ok); r.advance(100);
    CHECK(r.app.system().state() != PowerState::Off);
}

TEST(system_auto_power_off_after_inactivity) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().setup.autoOff = 1;                              // 1 minute
    r.advance(30000); r.tap(Action::Tele);                           // activity resets the timer
    r.advance(40000);                                                // 40 s since the last key press
    CHECK(r.app.system().state() == PowerState::On);
    r.advance(20500);                                                // 60.5 s since the last key press
    CHECK(r.app.system().state() == PowerState::ShuttingDown);
    r.advance(1500);
    CHECK(r.app.system().state() == PowerState::Off);
}

TEST(system_auto_power_off_can_be_disabled) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().setup.autoOff = 0;
    r.advance(20 * 60 * 1000);
    CHECK(r.app.system().state() == PowerState::On);
}

TEST(system_power_closes_open_dialog) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play); r.tap(Action::Delete);
    CHECK(r.app.ui().dialogOpen());
    r.input.tap(Action::Power); r.advance(60);
    CHECK(!r.app.ui().dialogOpen());
}

TEST(system_power_cycle_keeps_settings_and_state) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.mode = 1; r.app.settings().shoot.ev = 9;
    r.input.tap(Action::Power); r.advance(1500);
    r.input.tap(Action::Power); r.advance(100);
    CHECK_EQ(r.app.settings().shoot.mode, 1);
    CHECK_EQ(r.app.settings().shoot.ev, 9);
}
