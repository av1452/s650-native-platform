#pragma once
// Images and persistent settings. The desktop implementation is in memory (plus an optional settings file).
// The real camera's file system, internal NAND volume and settings storage are NOT understood yet.
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "core/Settings.h"
#include "display/Framebuffer.h"

namespace s650 {

struct Result {
    bool ok = true;
    std::string error;
    static Result success() { return {}; }
    static Result fail(const std::string& why) { Result r; r.ok = false; r.error = why; return r; }
};

struct ImageMeta { int iso = 100; std::string shutter = "1/250", aperture = "F2.8"; int focalMM = 35; int evIdx = 6; int wb = 0; bool flash = false; };

struct ImageRecord {
    std::string name, folder = "101_SONY";
    int w = 3072, h = 2304;
    bool fine = true, isProtected = false;
    uint64_t bytes = 0;
    int64_t taken = 0;                       // wall seconds
    ImageMeta meta;
    std::shared_ptr<const Framebuffer> pixels;   // 320x240 preview; real storage would hold a JPEG file
};

class StorageBackend {
public:
    virtual ~StorageBackend() = default;
    virtual bool initialize() = 0;
    virtual std::string description() const = 0;
    virtual const std::vector<ImageRecord>& images() const = 0;
    virtual Result addImage(const ImageRecord& r) = 0;                 // fails when full
    virtual Result setProtected(size_t index, bool on) = 0;
    virtual Result removeImage(size_t index) = 0;                      // refuses protected images
    virtual int removeAllUnprotected() = 0;                            // returns how many were removed
    virtual int format() = 0;                                          // erases everything; returns count
    virtual uint64_t capacityBytes() const = 0;
    virtual uint64_t usedBytes() const = 0;
    virtual uint64_t internalBytes() const { return 25263616; }       // observed size of the real internal volume (info only)
    virtual bool loadSettings(Settings& out) = 0;
    virtual Result saveSettings(const Settings& s) = 0;
    uint64_t freeBytes() const { return capacityBytes() > usedBytes() ? capacityBytes() - usedBytes() : 0; }
};

// In-memory images (16 procedural samples) with optional file-backed settings. SIMULATION only.
class SimulatedStorage : public StorageBackend {
public:
    explicit SimulatedStorage(std::string settingsPath = "", bool seedSamples = true, uint64_t capacity = 1000000000ull)
        : path_(std::move(settingsPath)), seed_(seedSamples), capacity_(capacity) {}
    bool initialize() override;
    std::string description() const override { return "Simulated storage (memory; 1 GB card model)"; }
    const std::vector<ImageRecord>& images() const override { return images_; }
    Result addImage(const ImageRecord& r) override;
    Result setProtected(size_t i, bool on) override;
    Result removeImage(size_t i) override;
    int removeAllUnprotected() override;
    int format() override;
    uint64_t capacityBytes() const override { return capacity_; }
    uint64_t usedBytes() const override;
    bool loadSettings(Settings& out) override;
    Result saveSettings(const Settings& s) override;
private:
    std::string path_;
    bool seed_;
    uint64_t capacity_;
    std::vector<ImageRecord> images_;
    int next_ = 101;
};
}  // namespace s650
