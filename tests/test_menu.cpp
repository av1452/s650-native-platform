#include "TestFramework.h"
#include "core/DateTime.h"
#include "core/DesktopRig.h"
#include "ui/Screens.h"

using namespace s650;

static MenuScreen& menu(DesktopRig& r) { return static_cast<MenuScreen&>(r.app.ui().screen(ScreenId::Menu)); }

// open the menu and go to category c, rows level
static void openCat(DesktopRig& r, int c) {
    if (r.app.ui().current() != ScreenId::Menu) r.tap(Action::Menu);
    r.app.ui().show(ScreenId::Menu, c);
}

TEST(menu_opens_and_exits_with_menu_button) {
    DesktopRig r; CHECK(r.startReady());
    CHECK(r.app.ui().current() == ScreenId::Shooting);
    r.tap(Action::Menu);
    CHECK(r.app.ui().current() == ScreenId::Menu);
    r.tap(Action::Menu);
    CHECK(r.app.ui().current() == ScreenId::Shooting);
}

TEST(menu_modern_navigation_levels) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Menu);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
    r.tap(Action::Left);
    CHECK(menu(r).level() == MenuScreen::Level::Rail);
    r.tap(Action::Down, 3);
    CHECK_EQ(menu(r).category(), 3);
    r.tap(Action::Right);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
    r.tap(Action::Back);
    CHECK(menu(r).level() == MenuScreen::Level::Rail);
    r.tap(Action::Back);                       // back from the rail leaves the menu
    CHECK(r.app.ui().current() == ScreenId::Shooting);
}

TEST(menu_category_wraps_both_ways) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Menu); r.tap(Action::Left);
    r.tap(Action::Up);
    CHECK_EQ(menu(r).category(), 7);
    r.tap(Action::Down);
    CHECK_EQ(menu(r).category(), 0);
}

TEST(menu_remembers_category_and_row) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Menu); r.tap(Action::Down, 2);       // row 2 of Camera
    r.tap(Action::Menu);                               // exit
    r.tap(Action::Menu);                               // re-enter without a category parameter
    CHECK_EQ(menu(r).category(), 0);
    CHECK_EQ(menu(r).row(), 2);
}

TEST(menu_auto_mode_locks_exposure_rows) {
    DesktopRig r; CHECK(r.startReady());
    CHECK_EQ(r.app.settings().shoot.mode, 0);
    openCat(r, 4);                                     // Exposure
    r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Rows); // slider did not open
    CHECK_EQ(r.app.settings().shoot.ev, 6);
    CHECK(r.app.ui().lastAnnouncement().find("Locked in Auto") != std::string::npos);
    openCat(r, 5); r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
    openCat(r, 6); r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
}

TEST(menu_picker_apply_and_cancel) {
    DesktopRig r; CHECK(r.startReady());
    openCat(r, 0);
    r.tap(Action::Ok);                                  // open Shooting mode picker
    CHECK(menu(r).level() == MenuScreen::Level::Picker);
    CHECK_EQ(menu(r).pickerIndex(), 0);
    r.tap(Action::Down);
    r.tap(Action::Back);                                // cancel
    CHECK_EQ(r.app.settings().shoot.mode, 0);
    r.tap(Action::Ok); r.tap(Action::Down); r.tap(Action::Ok);   // choose Program
    CHECK_EQ(r.app.settings().shoot.mode, 1);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
}

TEST(menu_picker_wraps) {
    DesktopRig r; CHECK(r.startReady());
    openCat(r, 0); r.tap(Action::Ok);
    r.tap(Action::Up);
    CHECK_EQ(menu(r).pickerIndex(), 3);
    r.tap(Action::Down);
    CHECK_EQ(menu(r).pickerIndex(), 0);
}

TEST(menu_slider_live_adjust_confirm_and_restore) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.mode = 1;                    // Program unlocks exposure
    openCat(r, 4); r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Slider);
    r.tap(Action::Right, 3);
    CHECK_EQ(r.app.settings().shoot.ev, 9);             // applied live
    r.tap(Action::Back);
    CHECK_EQ(r.app.settings().shoot.ev, 6);             // restored
    r.tap(Action::Ok); r.tap(Action::Right, 2); r.tap(Action::Ok);
    CHECK_EQ(r.app.settings().shoot.ev, 8);             // confirmed
    r.tap(Action::Ok); r.tap(Action::Left, 20);
    CHECK_EQ(r.app.settings().shoot.ev, 0);             // clamps at the end of the scale
}

TEST(menu_disabled_rows_explain_themselves) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.flash = 3;                   // flash off
    openCat(r, 3); r.tap(Action::Down); r.tap(Action::Ok);   // Red-eye reduction
    CHECK(r.app.settings().shoot.redeye);
    CHECK(r.app.ui().lastAnnouncement().find("Flash is set to Off") != std::string::npos);
    r.app.settings().shoot.flash = 0; r.app.settings().shoot.mode = 3;       // movie
    openCat(r, 1); r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
    CHECK(r.app.ui().lastAnnouncement().find("Movie") != std::string::npos);
}

TEST(menu_scene_row_only_in_scene_mode) {
    DesktopRig r; CHECK(r.startReady());
    openCat(r, 0); r.tap(Action::Down); r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
    r.app.settings().shoot.mode = 2;
    r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Picker);
}

TEST(menu_boolean_row_toggles_immediately) {
    DesktopRig r; CHECK(r.startReady());
    openCat(r, 2); r.tap(Action::Down);                 // Macro
    bool before = r.app.settings().shoot.macro;
    r.tap(Action::Ok);
    CHECK(r.app.settings().shoot.macro != before);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
}

TEST(menu_datetime_editor_commit_and_cancel) {
    DesktopRig r; CHECK(r.startReady());
    int64_t t0 = r.app.ui().wallNow();
    openCat(r, 7);                                       // Date & time is row 0 of Setup
    r.app.ui().show(ScreenId::Menu, 7);
    while (menu(r).row() != 0) r.tap(Action::Up);
    r.tap(Action::Ok);
    CHECK(menu(r).level() == MenuScreen::Level::Edit);
    r.tap(Action::Up);                                   // year + 1
    r.tap(Action::Back);                                 // cancel
    CHECK_EQ(r.app.ui().wallNow() / 86400, t0 / 86400);
    r.tap(Action::Ok); r.tap(Action::Up); r.tap(Action::Ok);
    int y0, m, d, h, mi; splitDateTime(t0, y0, m, d, h, mi);
    int y1; splitDateTime(r.app.ui().wallNow(), y1, m, d, h, mi);
    CHECK_EQ(y1, y0 + 1);
}

TEST(menu_initialize_dialog) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().shoot.timer = 2; r.app.settings().display.style = 1;
    openCat(r, 7);
    while (menu(r).row() != 10) r.tap(Action::Down);
    r.tap(Action::Ok);
    CHECK(r.app.ui().dialogOpen());
    r.tap(Action::Back);                                 // cancel keeps settings
    CHECK_EQ(r.app.settings().shoot.timer, 2);
    r.tap(Action::Ok);
    CHECK(r.app.ui().dialogOpen());
    r.tap(Action::Ok);                                   // default focus is OK for Initialize
    CHECK(!r.app.ui().dialogOpen());
    CHECK_EQ(r.app.settings().shoot.timer, 0);
    CHECK_EQ(r.app.settings().display.style, 0);
}

TEST(menu_format_dialog_defaults_to_cancel) {
    DesktopRig r; CHECK(r.startReady());
    size_t n = r.storage.images().size();
    CHECK(n > 0);
    openCat(r, 7);
    while (menu(r).row() != 11) r.tap(Action::Down);
    r.tap(Action::Ok);
    CHECK(r.app.ui().dialogOpen());
    CHECK(!r.app.ui().dialog().focusOk);
    r.tap(Action::Ok);                                   // Enter on the default: cancel
    CHECK_EQ(r.storage.images().size(), n);
    r.tap(Action::Ok); r.tap(Action::Right); r.tap(Action::Ok);   // move focus to Format and confirm
    CHECK_EQ(r.storage.images().size(), size_t(0));
}

TEST(menu_classic_navigation_is_horizontal) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().display.style = 2;
    r.tap(Action::Menu);
    r.tap(Action::Up);                                   // from first row up to the tabs
    CHECK(menu(r).level() == MenuScreen::Level::Rail);
    r.tap(Action::Right);
    CHECK_EQ(menu(r).category(), 1);
    r.tap(Action::Left, 2);
    CHECK_EQ(menu(r).category(), 7);
    r.tap(Action::Down);
    CHECK(menu(r).level() == MenuScreen::Level::Rows);
}

TEST(menu_setup_opens_display_options_and_returns) {
    DesktopRig r; CHECK(r.startReady());
    openCat(r, 7);
    while (menu(r).row() != 7) r.tap(Action::Down);
    r.tap(Action::Ok);
    CHECK(r.app.ui().current() == ScreenId::Display);
    r.tap(Action::Back);
    CHECK(r.app.ui().current() == ScreenId::Menu);
    CHECK_EQ(menu(r).category(), 7);
}

TEST(menu_changes_are_persisted_through_storage) {
    const char* path = "/tmp/s650_test_menu_persist.txt";
    std::remove(path);
    {
        DesktopRig r(path); CHECK(r.startReady());
        openCat(r, 0); r.tap(Action::Ok); r.tap(Action::Down, 2); r.tap(Action::Ok);   // Scene
    }
    SimulatedStorage st(path, false); st.initialize();
    Settings s; CHECK(st.loadSettings(s));
    CHECK_EQ(s.shoot.mode, 2);
    std::remove(path);
}

TEST(display_options_changes_apply_and_custom_state) {
    DesktopRig r; CHECK(r.startReady());
    r.app.ui().show(ScreenId::Display, 0);
    r.tap(Action::Down); r.tap(Action::Right);           // preset -> detailed
    CHECK_EQ(r.app.settings().display.density, 2);
    CHECK(r.app.settings().display.items.readout);
    r.tap(Action::Down); r.tap(Action::Ok);              // battery off
    CHECK(!r.app.settings().display.items.battery);
    CHECK_EQ(r.app.settings().display.density, 3);       // custom
    r.tap(Action::Up, 2); r.tap(Action::Ok);             // shooting info off
    CHECK(!r.app.settings().display.showInfo);
    for (int i = 0; i < 12; ++i) r.tap(Action::Down);    // appearance (row 12)
    r.tap(Action::Right);
    CHECK_EQ(r.app.settings().display.style, 1);
}
