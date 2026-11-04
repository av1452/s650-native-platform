#include "core/Translation.h"
#include "ui/MenuModel.h"
#include "core/OptionsText.h"
#include <algorithm>
#include "core/Clock.h"
#include "core/DateTime.h"
#include "core/Strings.h"

namespace s650 {
namespace {
std::string key(const char* group, const char* suffix) {
    FixedKey<64> translatedKey(group, ".", suffix);
    return translatedKey.valid()
        ? tr(translatedKey.c_str())
        : std::string();
}


using Opts = std::function<std::vector<Option>()>;

MenuRow choice(const std::string& id, int& ref, int count, std::function<std::string(int)> label,
               std::function<std::string(int)> descr = nullptr, std::function<std::string(int)> extra = nullptr,
               std::function<std::string()> disabled = nullptr) {
    MenuRow r;
    r.id = id; r.label = key("row", id.c_str()); r.descr = key("dr", id.c_str()); r.type = RowType::Choice; r.disabled = disabled;
    r.options = [=] {
        std::vector<Option> v;
        for (int i = 0; i < count; ++i) v.push_back({i, label(i), descr ? descr(i) : "", extra ? extra(i) : ""});
        return v;
    };
    r.current = [&ref] { return ref; };
    r.apply = [&ref](int v) { ref = v; };
    r.value = [&ref, label] { return label(ref); };
    return r;
}

MenuRow boolRow(const std::string& id, bool& ref, std::function<std::string()> disabled = nullptr) {
    MenuRow r;
    r.id = id; r.label = key("row", id.c_str()); r.descr = key("dr", id.c_str()); r.type = RowType::Bool; r.disabled = disabled;
    r.current = [&ref] { return ref ? 1 : 0; };
    r.apply = [&ref](int v) { ref = v != 0; };
    r.value = [&ref] { return tr(ref ? "opt.on" : "opt.off"); };
    return r;
}

MenuRow sliderRow(const std::string& id, int& ref, int count, int major, std::string lo, std::string hi,
                  std::function<std::string(int)> fmt, std::function<std::string()> disabled = nullptr) {
    MenuRow r;
    r.id = id; r.label = key("row", id.c_str()); r.descr = key("dr", id.c_str()); r.type = RowType::Slider; r.disabled = disabled;
    r.sliderCount = count; r.sliderMajor = major; r.lo = lo; r.hi = hi; r.sliderFmt = fmt;
    r.current = [&ref] { return ref; };
    r.apply = [&ref](int v) { ref = v; };
    r.value = [&ref, fmt] { return fmt(ref); };
    return r;
}

MenuRow actionRow(const std::string& id, ActionId a, bool danger = false) {
    MenuRow r;
    r.id = id; r.label = key("row", id.c_str()); r.descr = key("dr", id.c_str()); r.type = RowType::Action; r.action = a; r.danger = danger;
    r.value = [] { return std::string(); };
    return r;
}


}  // namespace

std::vector<MenuCategory> buildMenu(Settings& s, const StorageBackend& storage, int64_t wallNow) {
    auto& c = s.shoot;
    auto lockedInAuto = [&s]() -> std::string { return s.shoot.mode == 0 ? "why.program" : ""; };
    auto notMovie = [&s]() -> std::string { return s.shoot.mode == 3 ? "why.movie" : ""; };
    int freeBytes = int(std::min<uint64_t>(storage.freeBytes(), 2000000000ull));

    std::vector<MenuCategory> m;
    m.push_back({"camera", {
        choice("mode", c.mode, 4, [](int i) { return key("modename", kModeKeys[i]); }, [](int i) { return key("d.mode", kModeKeys[i]); }),
        choice("scene", c.scene, 4, [](int i) { return key("scene", kSceneKeys[i]); }, [](int i) { return key("d.scene", kSceneKeys[i]); }, nullptr,
               [&s]() -> std::string { return s.shoot.mode != 2 ? "why.scene" : ""; }),
        choice("timer", c.timer, 3, [](int i) { return key("timer", kTimerKeys[i]); }, [](int i) { return key("d.timer", kTimerKeys[i]); })}});
    m.push_back({"size", {
        choice("size", c.size, 6, [](int i) { return std::string(kSizes[i].id); },
               [](int i) { return std::to_string(kSizes[i].w) + " x " + std::to_string(kSizes[i].h) + ". " + tr("d.size"); },
               [&s, freeBytes](int i) { int per = s.shoot.quality == 0 ? kSizes[i].fineBytes : kSizes[i].stdBytes; return std::to_string(freeBytes / per) + " " + tr("unit.shots"); },
               notMovie),
        choice("quality", c.quality, 2, [](int i) { return key("quality", kQualityKeys[i]); }, [](int i) { return key("d.quality", kQualityKeys[i]); }, nullptr, notMovie)}});
    m.push_back({"focus", {
        choice("af", c.af, 2, [](int i) { return key("af", kAfKeys[i]); }, [](int i) { return key("d.af", kAfKeys[i]); }),
        boolRow("macro", c.macro)}});
    m.push_back({"flash", {
        choice("flash", c.flash, 4, [](int i) { return key("flash", kFlashKeys[i]); }, [](int i) { return key("d.flash", kFlashKeys[i]); }, nullptr, notMovie),
        boolRow("redeye", c.redeye, [&s]() -> std::string { return s.shoot.mode == 3 ? "why.movie" : (s.shoot.flash == 3 ? "why.flashoff" : ""); })}});
    m.push_back({"exposure", {
        sliderRow("ev", c.ev, kEvSteps, 3, "-2.0", "+2.0", [](int i) { return evLabel(i) + " EV"; }, lockedInAuto)}});
    m.push_back({"wb", {
        choice("wb", c.wb, 6, [](int i) { return key("wb", kWbKeys[i]); }, [](int i) { return key("d.wb", kWbKeys[i]); }, nullptr, lockedInAuto)}});
    m.push_back({"iso", {
        choice("iso", c.iso, 6, [](int i) { return i == 0 ? tr("iso.auto") : std::to_string(kIsoValues[i]); },
               [](int i) { return tr(i == 0 ? "d.iso.auto" : "d.iso.n"); }, nullptr, lockedInAuto)}});

    MenuRow dt;
    dt.id = "datetime"; dt.label = tr("row.datetime"); dt.descr = tr("dr.datetime"); dt.type = RowType::DateTime;
    dt.value = [wallNow] { return formatDateTime(wallNow); };
    auto& u = s.setup;
    MenuCategory setup{"setup", {}};
    setup.rows.push_back(dt);
    setup.rows.push_back(sliderRow("brightness", u.brightness, 5, 1, tr("lcd.dark"), tr("lcd.bright"), [](int v) { return std::to_string(v) + " / 5"; }));
    // brightness is 1..5 but slider indexes are 0..4: wrap with an offset
    {
        MenuRow& b = setup.rows.back();
        int* ref = &u.brightness;
        b.current = [ref] { return *ref - 1; };
        b.apply = [ref](int v) { *ref = v + 1; };
        b.sliderFmt = [](int i) { return std::to_string(i + 1) + " / 5"; };
    }
    setup.rows.push_back(choice("beep", u.beep, 3, [](int i) { return key("beep", kBeepKeys[i]); }, [](int i) { return key("d.beep", kBeepKeys[i]); }));
    {   // Language: only English exists natively (see docs/PORTABILITY.md); kept so the menu matches Version 2.0
        static int onlyEnglish = 0;
        setup.rows.push_back(choice("language", onlyEnglish, 1, [](int) { return tr("lang.en"); }));
    }
    setup.rows.push_back(choice("usb", u.usb, 3, [](int i) { return key("usb", kUsbKeys[i]); }, [](int i) { return key("d.usb", kUsbKeys[i]); }));
    setup.rows.push_back(boolRow("startup", u.startup));
    setup.rows.push_back(choice("autooff", u.autoOff, 4, [](int i) { return kAutoOffMinutes[i] ? std::to_string(kAutoOffMinutes[i]) + " " + tr("unit.min") : tr("opt.off"); }));
    setup.rows.push_back(actionRow("display", ActionId::OpenDisplay));
    setup.rows.push_back(actionRow("memory", ActionId::ViewMemory));
    setup.rows.push_back(actionRow("status", ActionId::ViewStatus));
    setup.rows.push_back(actionRow("initialize", ActionId::Initialize));
    setup.rows.push_back(actionRow("format", ActionId::Format, true));
    m.push_back(setup);
    return m;
}

}  // namespace s650
