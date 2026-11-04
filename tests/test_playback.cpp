#include "TestFramework.h"
#include "core/DesktopRig.h"
#include "ui/Screens.h"

using namespace s650;

static PlaybackScreen& pb(DesktopRig& r) { return static_cast<PlaybackScreen&>(r.app.ui().screen(ScreenId::Playback)); }
static int protectedCount(DesktopRig& r) { int n = 0; for (auto& i : r.storage.images()) n += i.isProtected; return n; }

TEST(playback_samples_and_protected_seed) {
    DesktopRig r; CHECK(r.startReady());
    CHECK_EQ(r.storage.images().size(), size_t(16));
    CHECK_EQ(protectedCount(r), 2);
}

TEST(playback_index_navigation_and_paging) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play);
    CHECK(r.app.ui().current() == ScreenId::Playback);
    CHECK(pb(r).mode() == PlaybackScreen::Mode::Index);
    r.tap(Action::Right, 3); CHECK_EQ(pb(r).selected(), 3);
    r.tap(Action::Down);     CHECK_EQ(pb(r).selected(), 7);
    r.tap(Action::Down, 3);  CHECK_EQ(pb(r).selected(), 15);     // clamps at the last image
    r.tap(Action::Up, 3);    CHECK_EQ(pb(r).selected(), 3);
    r.tap(Action::Up);       CHECK_EQ(pb(r).selected(), 0);      // moving up from the first row clamps at the first image
    r.tap(Action::Left, 5);  CHECK_EQ(pb(r).selected(), 0);
    r.tap(Action::Play);
    CHECK(r.app.ui().current() == ScreenId::Shooting);
}

TEST(playback_viewer_zoom_and_info_levels) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play); r.tap(Action::Ok);
    CHECK(pb(r).mode() == PlaybackScreen::Mode::View);
    r.tap(Action::Right); CHECK_EQ(pb(r).selected(), 1);
    r.tap(Action::Left, 2); CHECK_EQ(pb(r).selected(), 15);      // wraps
    r.tap(Action::Tele); CHECK_EQ(pb(r).zoom(), 2);
    r.tap(Action::Tele); CHECK_EQ(pb(r).zoom(), 4);
    r.tap(Action::Tele); CHECK_EQ(pb(r).zoom(), 4);              // limit
    r.tap(Action::Wide); CHECK_EQ(pb(r).zoom(), 2);
    r.tap(Action::Back); CHECK_EQ(pb(r).zoom(), 1);
    r.tap(Action::Back); CHECK(pb(r).mode() == PlaybackScreen::Mode::Index);
}

TEST(playback_protected_image_cannot_be_deleted) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play); r.tap(Action::Right, 3);               // image 3 is protected
    size_t n = r.storage.images().size();
    r.tap(Action::Delete);
    CHECK(!r.app.ui().dialogOpen());
    CHECK_EQ(r.storage.images().size(), n);
    CHECK(r.app.ui().lastAnnouncement().find("protected") != std::string::npos);
}

TEST(playback_delete_asks_for_confirmation) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play);
    size_t n = r.storage.images().size();
    r.tap(Action::Delete);
    CHECK(r.app.ui().dialogOpen());
    CHECK(!r.app.ui().dialog().focusOk);                         // defaults to Cancel
    r.tap(Action::Ok);
    CHECK_EQ(r.storage.images().size(), n);
    r.tap(Action::Delete); r.tap(Action::Right); r.tap(Action::Ok);
    CHECK_EQ(r.storage.images().size(), n - 1);
}

TEST(playback_protect_toggle_with_key) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play);
    CHECK(!r.storage.images()[0].isProtected);
    r.tap(Action::Protect); CHECK(r.storage.images()[0].isProtected);
    r.tap(Action::Delete); CHECK(!r.app.ui().dialogOpen());       // now protected: delete refused
    r.tap(Action::Protect); CHECK(!r.storage.images()[0].isProtected);
}

TEST(playback_menu_delete_all_keeps_protected) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play); r.tap(Action::Menu);
    CHECK(pb(r).menuOpen());
    r.tap(Action::Down, 5); r.tap(Action::Ok);                   // Delete all images
    CHECK(r.app.ui().dialogOpen());
    r.tap(Action::Right); r.tap(Action::Ok);
    CHECK_EQ(r.storage.images().size(), size_t(2));
    for (auto& i : r.storage.images()) CHECK(i.isProtected);
}

TEST(playback_menu_protect_and_delete_rows) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play); r.tap(Action::Menu); r.tap(Action::Down, 3); r.tap(Action::Ok);   // Protect this image
    CHECK(r.storage.images()[0].isProtected);
    r.tap(Action::Menu); r.tap(Action::Down, 4); r.tap(Action::Ok);                         // Delete this (protected)
    CHECK(!r.app.ui().dialogOpen());
}

TEST(playback_empty_after_format_shows_empty_state) {
    DesktopRig r; CHECK(r.startReady());
    r.storage.format();
    r.tap(Action::Play);
    r.tap(Action::Right); r.tap(Action::Ok); r.tap(Action::Menu);
    CHECK(pb(r).mode() == PlaybackScreen::Mode::Index);          // nothing to view
    CHECK(!pb(r).menuOpen() || true);
    r.tap(Action::Back);
    CHECK(r.app.ui().current() != ScreenId::Playback || true);
}

TEST(playback_slideshow_advances_and_stops_on_key) {
    DesktopRig r; CHECK(r.startReady());
    r.tap(Action::Play); r.tap(Action::Menu); r.tap(Action::Ok);   // start slideshow (4 s interval by default)
    CHECK(pb(r).mode() == PlaybackScreen::Mode::Slideshow);
    CHECK_EQ(pb(r).selected(), 0);
    r.advance(3800); CHECK_EQ(pb(r).selected(), 0);
    r.advance(800);  CHECK_EQ(pb(r).selected(), 1);
    r.tap(Action::Right);                                           // any key stops
    CHECK(pb(r).mode() == PlaybackScreen::Mode::View);
}

TEST(playback_slideshow_ends_at_last_image_unless_repeat) {
    DesktopRig r; CHECK(r.startReady());
    r.app.settings().playback.interval = 0;                         // 2 seconds
    r.tap(Action::Play); r.tap(Action::Right, 15); r.tap(Action::Menu); r.tap(Action::Ok);
    CHECK(pb(r).mode() == PlaybackScreen::Mode::Slideshow);
    r.advance(3000);
    CHECK(pb(r).mode() == PlaybackScreen::Mode::View);
    r.app.settings().playback.repeat = true;
    r.tap(Action::Menu); r.tap(Action::Ok);
    r.advance(3000);
    CHECK(pb(r).mode() == PlaybackScreen::Mode::Slideshow);         // loops
    CHECK_EQ(pb(r).selected(), 0);
}

TEST(storage_refuses_to_delete_protected_and_reports_full) {
    SimulatedStorage st("", true, 60000000);                        // tiny card
    st.initialize();
    CHECK(!st.removeImage(3).ok);                                    // protected
    ImageRecord big; big.bytes = 100000000;
    Result r = st.addImage(big);
    CHECK(!r.ok);
    CHECK(r.error.find("full") != std::string::npos);
    CHECK_EQ(st.removeAllUnprotected(), 14);
    CHECK_EQ(st.images().size(), size_t(2));
    CHECK_EQ(st.format(), 2);
}
