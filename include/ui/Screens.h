#pragma once
#include "ui/Hud.h"
#include "ui/MenuModel.h"
#include "ui/UIController.h"

namespace s650 {

class ShootingScreen : public Screen {
public:
    explicit ShootingScreen(UIController& ui) : ui_(ui) {}
    void enter(int param) override;
    void exit() override;
    void onInput(const InputEvent& e) override;
    void update(uint32_t dtMs) override;
    void render(Canvas& cv) override;
    // observable state (tests, status display)
    FocusState focus() const { return focus_; }
    bool recording() const { return recording_; }
    int countdownSeconds() const { return countdownMs_ < 0 ? -1 : (countdownMs_ + 999) / 1000; }
    const std::string& toastText() const { return toast_; }
private:
    void toast(const std::string& t);
    void capture();
    void beginShot();
    void cycleDisplay();
    void cancelCountdown();
    UIController& ui_;
    Framebuffer live_;
    bool haveLive_ = false;
    std::string liveError_;
    FocusState focus_ = FocusState::Idle;
    bool halfDown_ = false, recording_ = false;
    int afMs_ = 0, countdownMs_ = -1, toastMs_ = 0, histMs_ = 0, flashMs_ = 0, flashPeak_ = 0, preflashStep_ = -1, preflashMs_ = 0, settleMs_ = -1;
    uint64_t time_ = 0, recStart_ = 0;
    bool pendingShot_ = false;
    std::string toast_;
    std::vector<float> hist_;
};

class MenuScreen : public Screen {
public:
    explicit MenuScreen(UIController& ui) : ui_(ui) {}
    void enter(int param) override;
    void onInput(const InputEvent& e) override;
    void update(uint32_t dtMs) override;
    void render(Canvas& cv) override;
    enum class Level { Rail, Rows, Picker, Slider, Edit, View };
    Level level() const { return level_; }
    int category() const { return cat_; }
    int row() const { return rowIdx(); }
    int pickerIndex() const { return pickIndex_; }
private:
    int rowIdx() const { return rows_[size_t(cat_)]; }
    MenuRow& currentRow(std::vector<MenuCategory>& m);
    void say(const std::vector<MenuCategory>& m);
    void activate(std::vector<MenuCategory>& m, MenuRow& r);
    void flash(const std::string& text, bool warn);
    void commitEdit();
    UIController& ui_;
    int cat_ = 0, rows_[8] = {0, 0, 0, 0, 0, 0, 0, 0}, scroll_[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    Level level_ = Level::Rows;
    int pickIndex_ = 0, pickOriginal_ = 0, sliderIndex_ = 0, sliderOriginal_ = 0;
    int edit_[5] = {2007, 1, 1, 0, 0}, editField_ = 0;
    std::string view_, msg_;
    bool msgWarn_ = false;
    int msgMs_ = 0;
};

class DisplayOptionsScreen : public Screen {
public:
    explicit DisplayOptionsScreen(UIController& ui) : ui_(ui) {}
    void enter(int) override { row_ = 0; scroll_ = 0; }
    void onInput(const InputEvent& e) override;
    void render(Canvas& cv) override;
    int row() const { return row_; }
    static constexpr int kRows = 13;
private:
    UIController& ui_;
    int row_ = 0, scroll_ = 0;
};

class PlaybackScreen : public Screen {
public:
    explicit PlaybackScreen(UIController& ui) : ui_(ui) {}
    void enter(int param) override;
    void exit() override;
    void onInput(const InputEvent& e) override;
    void update(uint32_t dtMs) override;
    void render(Canvas& cv) override;
    enum class Mode { Index, View, Slideshow };
    Mode mode() const { return mode_; }
    int selected() const { return sel_; }
    int zoom() const { return zoom_; }
    bool menuOpen() const { return menuOpen_; }
private:
    void toast(const std::string& t);
    void askDelete();
    void askDeleteAll();
    void toggleProtect();
    void menuInput(Action a);
    void stopSlideshow(const std::string& msg);
    void renderIndex(Canvas& cv);
    void renderView(Canvas& cv);
    void drawBig(Canvas& cv, size_t idx, int alpha);
    UIController& ui_;
    Mode mode_ = Mode::Index;
    int sel_ = 0, info_ = 0, zoom_ = 1, px_ = 0, py_ = 0, menuRow_ = 0, toastMs_ = 0, ssMs_ = 0, fadeMs_ = -1, fadeTo_ = 0;
    bool menuOpen_ = false;
    std::string toast_;
};

}  // namespace s650
