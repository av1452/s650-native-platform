#include "TestFramework.h"
#include "camera/CameraBackend.h"
#include "camera/Exposure.h"
#include "core/Clock.h"
#include "core/DateTime.h"
#include "core/Strings.h"
#include "input/InputBackend.h"
#include "storage/StorageBackend.h"

using namespace s650;

TEST(camera_unavailable_reports_and_fails_safely) {
    UnavailableCamera cam;
    CHECK(!cam.initialize());
    CameraStatus s = cam.status();
    CHECK(!s.available); CHECK(!s.simulated);
    Framebuffer fb; Effective e = effective(Settings());
    CHECK(!cam.liveFrame(fb, e, 0));
    CHECK(!cam.exposure(e).valid);
    CaptureResult c = cam.capture(e, 0);
    CHECK(!c.status.ok); CHECK(c.image == nullptr);
    CHECK(!cam.setMovieRecording(true).ok);
    CHECK(!cam.flashWouldFire(e));
}

TEST(camera_simulated_labels_itself_as_simulation) {
    SimulatedCamera cam;
    CHECK(cam.initialize());
    CameraStatus s = cam.status();
    CHECK(s.available); CHECK(s.simulated);
    CHECK(s.description.find("Simulated") != std::string::npos);
}

TEST(camera_simulated_exposure_responds_to_settings) {
    SimulatedCamera cam;
    Settings s; s.shoot.mode = 1;
    Effective base = effective(s);
    ExposureInfo a = cam.exposure(base);
    CHECK(a.valid); CHECK(a.shutter.find("1/") == 0); CHECK(a.aperture[0] == 'F');
    s.shoot.ev = 12; ExposureInfo brighter = cam.exposure(effective(s));
    CHECK(brighter.shutterSeconds > a.shutterSeconds);              // +2 EV lengthens the exposure
    s.shoot.ev = 6; s.shoot.zoom = 25; ExposureInfo tele = cam.exposure(effective(s));
    CHECK(tele.fNumber > a.fNumber);                                 // smaller aperture at tele
    s.shoot.iso = 5;
    CHECK_EQ(cam.exposure(effective(s)).iso, 1000);
}

TEST(camera_simulated_flash_rules) {
    SimulatedCamera cam;
    Settings s; s.shoot.mode = 1;
    s.shoot.flash = 1; CHECK(cam.flashWouldFire(effective(s)));     // forced on
    s.shoot.flash = 3; CHECK(!cam.flashWouldFire(effective(s)));    // off
    s.shoot.flash = 0; CHECK(!cam.flashWouldFire(effective(s)));    // auto in bright lake scene
    s.shoot.mode = 2; s.shoot.scene = 2; CHECK(cam.flashWouldFire(effective(s)));   // twilight city: dark, auto fires
}

TEST(camera_simulated_capture_returns_image_and_meta) {
    SimulatedCamera cam;
    Settings s; s.shoot.mode = 1; s.shoot.zoom = 10;
    CaptureResult r = cam.capture(effective(s), 1177754400);
    CHECK(r.status.ok); CHECK(r.image != nullptr);
    CHECK_EQ(r.image->width(), 320);
    CHECK(r.meta.focalMM > 35);
}

TEST(strings_lookup_and_fallback) {
    CHECK_EQ(tr("mode.auto"), std::string("AUTO"));
    CHECK_EQ(tr("no.such.key"), std::string("no.such.key"));
    CHECK(kEnglishCount > 250);
}

TEST(input_key_mapping) {
    CHECK(actionForKey(1000, 0) == Action::Up);
    CHECK(actionForKey(1004, 0) == Action::Ok);
    CHECK(actionForKey(1005, 0) == Action::Back);
    CHECK(actionForKey(0, 'm') == Action::Menu);
    CHECK(actionForKey(0, 'M') == Action::Menu);
    CHECK(actionForKey(0, ' ') == Action::Shutter);
    CHECK(actionForKey(0, 'o') == Action::Power);
    CHECK(actionForKey(0, 'q') == Action::None);
}

TEST(scripted_input_queue) {
    ScriptedInput in;
    InputEvent e;
    CHECK(!in.poll(e));
    in.tap(Action::Shutter); in.tap(Action::Up);
    CHECK(in.poll(e)); CHECK(e.action == Action::Shutter && e.phase == Phase::Press);
    CHECK(in.poll(e)); CHECK(e.action == Action::Shutter && e.phase == Phase::Release);
    CHECK(in.poll(e)); CHECK(e.action == Action::Up);
    CHECK(!in.poll(e));
}

TEST(clock_formatting_and_normalisation) {
    ManualClock c;                                                   // 2007-04-28 10:00:00
    CHECK_EQ(formatDateTime(c.wallSeconds()), std::string("2007-04-28 10:00"));
    c.advance(61000);
    CHECK_EQ(formatTime(c.wallSeconds()), std::string("10:01"));
    int64_t t = joinDateTime(2007, 2, 31, 25, 0);                    // normalised, not rejected
    int y, mo, d, h, mi; splitDateTime(t, y, mo, d, h, mi);
    CHECK_EQ(y, 2007); CHECK_EQ(mo, 3); CHECK_EQ(d, 4); CHECK_EQ(h, 1);
}

TEST(storage_simulated_basics) {
    SimulatedStorage st("", true);
    CHECK(st.initialize());
    CHECK_EQ(st.images().size(), size_t(16));
    CHECK(st.usedBytes() > 20000000);
    CHECK(st.freeBytes() < st.capacityBytes());
    CHECK(st.setProtected(0, true).ok);
    CHECK(!st.setProtected(99, true).ok);
    CHECK(!st.removeImage(99).ok);
    CHECK_EQ(st.internalBytes(), uint64_t(25263616));
    SimulatedStorage empty("", false);
    empty.initialize();
    CHECK_EQ(empty.images().size(), size_t(0));
}

TEST(translation_allocation_free_lookup) {
    CHECK_EQ(std::string(translate("mode.auto")), std::string("AUTO"));
    CHECK_EQ(std::string(translate("no.such.key")),
             std::string("no.such.key"));
    CHECK(translate(nullptr)[0] == '\0');
    CHECK(translate("")[0] == '\0');
    CHECK(kEnglishCount == 303);
}

TEST(translation_table_has_no_duplicate_keys) {
    for (size_t i = 0; i < kEnglishCount; ++i) {
        CHECK(kEnglish[i].key != nullptr);
        CHECK(kEnglish[i].value != nullptr);

        for (size_t j = i + 1; j < kEnglishCount; ++j) {
            CHECK(std::string(kEnglish[i].key) !=
                  std::string(kEnglish[j].key));
        }
    }
}

TEST(fixed_translation_key_composition) {
    FixedKey<32> key("row", ".", "datetime");

    CHECK(key.valid());
    CHECK_EQ(std::string(key.c_str()), std::string("row.datetime"));
    CHECK_EQ(key.size(), size_t(12));
    CHECK_EQ(FixedKey<32>::capacity(), size_t(32));
}

TEST(fixed_translation_key_capacity) {
    FixedKey<5> exact("ab", "", "cd");
    CHECK(exact.valid());
    CHECK_EQ(std::string(exact.c_str()), std::string("abcd"));

    FixedKey<5> overflow("abc", "", "de");
    CHECK(!overflow.valid());
    CHECK_EQ(overflow.size(), size_t(0));
    CHECK_EQ(overflow.c_str()[0], '\0');
}

TEST(fixed_translation_key_rejects_null_parts) {
    FixedKey<16> invalid("row", ".", nullptr);

    CHECK(!invalid.valid());
    CHECK_EQ(invalid.c_str()[0], '\0');
}
