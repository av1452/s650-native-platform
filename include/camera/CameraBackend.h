#pragma once
// Everything the interface needs from the camera proper. NOTHING here is implemented against real S650 hardware:
// register maps, drivers and the original firmware's functions are unknown. Two implementations exist:
//   SimulatedCamera   - procedural live view, a toy exposure model, fake capture. Clearly labelled as simulated.
//   UnavailableCamera - every call fails visibly. This is what a not-yet-written hardware port must replace.
#include <memory>
#include <string>
#include "core/Settings.h"
#include "display/Framebuffer.h"
#include "display/Scenes.h"
#include "storage/StorageBackend.h"

namespace s650 {

struct CameraStatus {
    bool available = false;        // can this backend take pictures at all?
    bool simulated = false;        // true when values are invented by the desktop simulation
    std::string description;
    int batteryPercent = -1;       // -1 = unknown
};

struct ExposureInfo {
    bool valid = false;
    int iso = 0;
    std::string shutter = "--", aperture = "--";
    float shutterSeconds = 0, fNumber = 0;
};

struct CaptureResult {
    Result status;
    std::shared_ptr<Framebuffer> image;
    ImageMeta meta;
    bool flashFired = false;
};

class CameraBackend {
public:
    virtual ~CameraBackend() = default;
    virtual bool initialize() = 0;
    virtual CameraStatus status() const = 0;
    // Fills `out` (320x240) with the current preview. Returns false when there is no live view.
    virtual bool liveFrame(Framebuffer& out, const Effective& eff, uint64_t timeMs) = 0;
    virtual ExposureInfo exposure(const Effective& eff) const = 0;
    virtual bool flashWouldFire(const Effective& eff) const = 0;
    virtual CaptureResult capture(const Effective& eff, int64_t wallSeconds) = 0;
    virtual Result setMovieRecording(bool on) = 0;
};

class SimulatedCamera : public CameraBackend {
public:
    bool initialize() override { return true; }
    CameraStatus status() const override { return {true, true, "Simulated camera (procedural scenes, invented exposure values)", batteryPercent_}; }
    bool liveFrame(Framebuffer& out, const Effective& eff, uint64_t timeMs) override;
    ExposureInfo exposure(const Effective& eff) const override;
    bool flashWouldFire(const Effective& eff) const override;
    CaptureResult capture(const Effective& eff, int64_t wallSeconds) override;
    Result setMovieRecording(bool on) override { recording_ = on; return Result::success(); }
    void setBattery(int pct) { batteryPercent_ = pct; }
    bool recording() const { return recording_; }
private:
    int batteryPercent_ = 82;
    bool recording_ = false;
    uint32_t frame_ = 0;
};

class UnavailableCamera : public CameraBackend {
public:
    bool initialize() override { return false; }
    CameraStatus status() const override { return {false, false, "No camera backend: real camera access is not implemented", -1}; }
    bool liveFrame(Framebuffer&, const Effective&, uint64_t) override { return false; }
    ExposureInfo exposure(const Effective&) const override { return {}; }
    bool flashWouldFire(const Effective&) const override { return false; }
    CaptureResult capture(const Effective&, int64_t) override { CaptureResult r; r.status = Result::fail("Camera unavailable: no hardware backend"); return r; }
    Result setMovieRecording(bool) override { return Result::fail("Camera unavailable: no hardware backend"); }
};

Scene sceneFor(const Effective& eff);
}  // namespace s650
