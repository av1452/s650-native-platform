#include <cstdio>
#include <fstream>
#include "TestFramework.h"
#include "core/SettingsIO.h"
#include "core/OptionsText.h"
#include "core/Settings.h"
#include "storage/StorageBackend.h"

using namespace s650;

TEST(settings_defaults_roundtrip) {
    Settings a, b;
    std::string text = serialize(a);
    CHECK(deserialize(text, b));
    CHECK_EQ(serialize(b), text);
}

TEST(settings_roundtrip_with_changes) {
    Settings a, b;
    a.shoot.mode = 2; a.shoot.scene = 3; a.shoot.ev = 9; a.shoot.macro = true; a.shoot.redeye = false;
    a.display.style = 2; a.display.histogram = true; a.display.grid = 3; a.display.items.exposure = 2; a.display.density = 3;
    a.setup.clockOffset = -86400; a.setup.autoOff = 1; a.playback.repeat = true; a.playback.interval = 2;
    CHECK(deserialize(serialize(a), b));
    CHECK_EQ(b.shoot.mode, 2); CHECK_EQ(b.shoot.ev, 9); CHECK(b.shoot.macro); CHECK(!b.shoot.redeye);
    CHECK_EQ(b.display.style, 2); CHECK(b.display.histogram); CHECK_EQ(b.display.grid, 3); CHECK_EQ(b.display.items.exposure, 2);
    CHECK_EQ(b.setup.clockOffset, -86400); CHECK_EQ(b.setup.autoOff, 1); CHECK(b.playback.repeat); CHECK_EQ(b.playback.interval, 2);
}

TEST(settings_rejects_garbage) {
    Settings s; s.shoot.mode = 3;
    CHECK(!deserialize("", s));
    CHECK(!deserialize("hello world\nmode=1\n", s));
    CHECK_EQ(s.shoot.mode, 3);          // unchanged on failure
}

TEST(settings_clamps_and_ignores_unknown) {
    Settings s;
    CHECK(deserialize("S650SETTINGS 1\nmode=99\nev=-5\nbrightness=0\nbogus=7\nzoom=abc\nstyle=2\n", s));
    CHECK_EQ(s.shoot.mode, 3);          // clamped to max
    CHECK_EQ(s.shoot.ev, 0);            // clamped to min
    CHECK_EQ(s.setup.brightness, 1);
    CHECK_EQ(s.shoot.zoom, 0);          // unparsable line ignored -> default
    CHECK_EQ(s.display.style, 2);
}

TEST(settings_missing_keys_use_defaults) {
    Settings s;
    CHECK(deserialize("S650SETTINGS 1\nmode=1\n", s));
    CHECK_EQ(s.shoot.mode, 1);
    CHECK_EQ(s.setup.brightness, 3);
    CHECK(s.display.showInfo);
}

TEST(settings_initialize_keeps_clock) {
    Settings s; s.shoot.flash = 2; s.display.style = 1; s.setup.clockOffset = 12345; s.setup.beep = 2;
    initializeSettings(s);
    CHECK_EQ(s.shoot.flash, 0); CHECK_EQ(s.display.style, 0); CHECK_EQ(s.setup.beep, 1);
    CHECK_EQ(s.setup.clockOffset, 12345);
}

TEST(settings_effective_auto_lock_and_movie) {
    Settings s; s.shoot.ev = 10; s.shoot.wb = 3; s.shoot.iso = 4; s.shoot.flash = 1;
    s.shoot.mode = 0;
    Effective e = effective(s);
    CHECK_EQ(e.ev, 6); CHECK_EQ(e.wb, 0); CHECK_EQ(e.iso, 0);
    s.shoot.mode = 1;
    e = effective(s);
    CHECK_EQ(e.ev, 10); CHECK_EQ(e.wb, 3); CHECK_EQ(e.iso, 4);
    s.shoot.mode = 3;
    CHECK_EQ(effective(s).flash, 3);    // movie: flash off
}

TEST(settings_density_presets) {
    Settings s;
    applyDensity(s, 0); CHECK(!s.display.items.size); CHECK_EQ(s.display.items.exposure, 0);
    applyDensity(s, 2); CHECK(s.display.items.readout); CHECK_EQ(s.display.items.exposure, 2);
    applyDensity(s, 1); CHECK(s.display.items.size); CHECK(!s.display.items.readout); CHECK_EQ(s.display.density, 1);
}

TEST(settings_ev_labels) {
    CHECK_EQ(evLabel(6), std::string("0.0"));
    CHECK_EQ(evLabel(8), std::string("+0.7"));
    CHECK_EQ(evLabel(0), std::string("-2.0"));
}

TEST(settings_persist_to_file) {
    const char* path = "/tmp/s650_test_settings.txt";
    std::remove(path);
    Settings a; a.shoot.mode = 1; a.display.style = 2; a.setup.brightness = 5;
    {
        SimulatedStorage st(path, false);
        CHECK(st.initialize());
        Settings none;
        CHECK(!st.loadSettings(none));          // nothing saved yet
        CHECK(st.saveSettings(a).ok);
    }
    SimulatedStorage st2(path, false);
    st2.initialize();
    Settings b;
    CHECK(st2.loadSettings(b));
    CHECK_EQ(serialize(a), serialize(b));
    std::remove(path);
}

TEST(settings_persist_failure_is_reported) {
    SimulatedStorage st("/nonexistent_dir_s650/settings.txt", false);
    st.initialize();
    Result r = st.saveSettings(Settings());
    CHECK(!r.ok);
    CHECK(!r.error.empty());
}

TEST(settings_corrupt_file_is_ignored) {
    const char* path = "/tmp/s650_test_corrupt.txt";
    { std::ofstream f(path); f << "\x01\x02 not a settings file"; }
    SimulatedStorage st(path, false);
    st.initialize();
    Settings s; s.shoot.mode = 2;
    CHECK(!st.loadSettings(s));
    CHECK_EQ(s.shoot.mode, 2);
    std::remove(path);
}
