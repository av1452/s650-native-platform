#include "storage/StorageBackend.h"
#include "core/SettingsIO.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include "display/Scenes.h"

namespace s650 {

bool SimulatedStorage::initialize() {
    images_.clear();
    next_ = 101;
    if (!seed_) return true;
    static const Scene order[8] = {Scene::Lake, Scene::Sunset, Scene::City, Scene::Flower, Scene::Forest, Scene::Snow, Scene::Dunes, Scene::Harbor};
    static const int sizes[8] = {0, 2, 3, 0, 1, 2, 4, 2};
    const int64_t t0 = 1177754400;   // 2007-04-28 10:00
    for (int i = 0; i < 16; ++i) {
        ImageRecord r;
        char nm[24]; std::snprintf(nm, sizeof nm, "DSC00%d.JPG", next_++);
        r.name = nm;
        const SizeOption& z = kSizes[sizes[i % 8]];
        r.w = z.w; r.h = z.h; r.fine = i % 3 != 1;
        r.bytes = uint64_t(double(r.fine ? z.fineBytes : z.stdBytes) * (0.9 + 0.2 * double((i * 37) % 10) / 10.0));
        r.taken = t0 + int64_t(i) * 31 * 3600 + (i * 7919) % 5000;
        r.isProtected = i == 3 || i == 10;            // two protected samples so the lock indication is visible
        auto fb = std::make_shared<Framebuffer>();
        renderScene(*fb, order[i % 8], i / 8);
        r.pixels = fb;
        r.meta.iso = 100 + (i % 3) * 100; r.meta.shutter = i % 2 ? "1/125" : "1/500"; r.meta.aperture = "F2.9"; r.meta.focalMM = 35 + (i % 5) * 7;
        images_.push_back(r);
    }
    return true;
}

uint64_t SimulatedStorage::usedBytes() const { uint64_t n = 0; for (auto& r : images_) n += r.bytes; return n; }

Result SimulatedStorage::addImage(const ImageRecord& in) {
    if (in.bytes > freeBytes()) return Result::fail("Memory full");
    ImageRecord r = in;
    if (r.name.empty()) { char nm[24]; std::snprintf(nm, sizeof nm, "DSC00%d.JPG", next_++); r.name = nm; }
    images_.push_back(r);
    return Result::success();
}

Result SimulatedStorage::setProtected(size_t i, bool on) {
    if (i >= images_.size()) return Result::fail("No such image");
    images_[i].isProtected = on;
    return Result::success();
}

Result SimulatedStorage::removeImage(size_t i) {
    if (i >= images_.size()) return Result::fail("No such image");
    if (images_[i].isProtected) return Result::fail("Image is protected");
    images_.erase(images_.begin() + long(i));
    return Result::success();
}

int SimulatedStorage::removeAllUnprotected() {
    int n = 0;
    for (size_t i = images_.size(); i-- > 0;) if (!images_[i].isProtected) { images_.erase(images_.begin() + long(i)); ++n; }
    return n;
}

int SimulatedStorage::format() { int n = int(images_.size()); images_.clear(); return n; }

bool SimulatedStorage::loadSettings(Settings& out) {
    if (path_.empty()) return false;
    std::ifstream f(path_);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf();
    return deserialize(ss.str(), out);
}

Result SimulatedStorage::saveSettings(const Settings& s) {
    if (path_.empty()) return Result::success();     // nothing to persist to; not an error on the desktop
    std::string tmp = path_ + ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        if (!f) return Result::fail("Cannot write settings file");
        f << serialize(s);
        if (!f) return Result::fail("Write failed");
    }
    if (std::rename(tmp.c_str(), path_.c_str()) != 0) return Result::fail("Cannot replace settings file");
    return Result::success();
}

}  // namespace s650
