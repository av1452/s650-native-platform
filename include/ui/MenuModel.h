#pragma once
// Data model of the main menu and Setup (port of js/menu-def.js). Rows read and write Settings only; anything that
// would act on real camera hardware goes through the CameraBackend/StorageBackend, never directly.
#include <functional>
#include <string>
#include <vector>
#include "core/Settings.h"
#include "storage/StorageBackend.h"

namespace s650 {

enum class RowType { Choice, Bool, Slider, DateTime, Action };
enum class ActionId { None, OpenDisplay, ViewMemory, ViewStatus, Initialize, Format };

struct Option { int id; std::string label, descr, extra; };

struct MenuRow {
    std::string id, label, descr;
    RowType type = RowType::Choice;
    ActionId action = ActionId::None;
    bool danger = false;
    std::function<std::string()> disabled;                  // returns a string key explaining why, or empty
    std::function<std::vector<Option>()> options;           // Choice
    std::function<int()> current;                           // Choice: option id; Bool: 0/1; Slider: index
    std::function<void(int)> apply;
    int sliderCount = 0, sliderMajor = 1;                   // Slider
    std::string lo, hi;
    std::function<std::string(int)> sliderFmt;
    std::function<std::string()> value;                     // text shown at the right of the row

    std::string disabledReason() const { return disabled ? disabled() : std::string(); }
};

struct MenuCategory { std::string id; std::vector<MenuRow> rows; };

// 8 categories in order: Camera, Image Size, Focus, Flash, Exposure, White Balance, ISO, Setup
std::vector<MenuCategory> buildMenu(Settings& s, const StorageBackend& storage, int64_t wallNow);

}  // namespace s650
