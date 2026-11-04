#include "camera/CameraBackend.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "camera/Exposure.h"

namespace s650 {

Scene sceneFor(const Effective& e) {
    if (e.mode == 2) { static const Scene m[4] = {Scene::Flower, Scene::Lake, Scene::City, Scene::Harbor}; return m[std::clamp(e.scene, 0, 3)]; }
    return e.mode == 3 ? Scene::Harbor : Scene::Lake;
}

static int autoIso(int ev) { return ev >= 13 ? 100 : ev >= 11 ? 200 : ev >= 9 ? 400 : 800; }

ExposureModel modelExposure(int ev100, const Effective& e) {
    const float fWide = 2.8f, fTele = 4.8f;
    int iso = kIsoValues[std::clamp(e.iso, 0, 5)];
    if (iso == 0) iso = autoIso(ev100);
    float zoom = float(e.zoom) / 25.f;
    float f = fWide + zoom * (fTele - fWide);
    float evs = float(ev100) + std::log2(float(iso) / 100.f);
    float t = f * f / std::pow(2.f, evs) * std::pow(2.f, evValue(e.ev));
    return {iso, f, std::clamp(t, 1.f / 1000.f, 0.5f)};
}

std::string shutterText(float t) {
    if (t >= 1.f) return "1\"";
    static const int series[] = {2, 4, 8, 15, 30, 60, 125, 250, 500, 1000};
    float d = 1.f / t, bestD = 1e9f; int best = series[0];
    for (int s : series) { float diff = std::fabs(std::log(d / float(s))); if (diff < bestD) { bestD = diff; best = s; } }
    return "1/" + std::to_string(best);
}

void applyLook(Framebuffer& fb, const Effective& e, bool flashed, uint32_t seed) {
    static const float wb[6][3] = {{1, 1, 1}, {1.03f, 1, .97f}, {1.1f, 1.02f, .9f}, {1.06f, .94f, 1.06f}, {.84f, .98f, 1.22f}, {1.04f, 1, .98f}};
    int iso = kIsoValues[std::clamp(e.iso, 0, 5)];
    if (iso == 0) iso = 100;
    float gain = std::pow(2.f, evValue(e.ev) * 0.5f) * (flashed ? 1.08f : 1.f);
    float sigma = 2.f + 12.5f * std::log10(float(iso) / 100.f);
    const float* m = wb[std::clamp(e.wb, 0, 5)];
    uint32_t s = seed * 2654435761u + 1;
    for (int i = 0, n = fb.width() * fb.height(); i < n; ++i) {
        s = s * 1664525u + 1013904223u;
        float nz = (float(s >> 8) / 16777216.f - 0.5f) * 2.f * sigma;
        uint32_t p = fb.data()[i];
        auto ch = [&](uint32_t v, float mul) { return uint32_t(std::clamp(float(v) * mul * gain + nz, 0.f, 255.f)); };
        fb.data()[i] = 0xFF000000u | (ch((p >> 16) & 255, m[0]) << 16) | (ch((p >> 8) & 255, m[1]) << 8) | ch(p & 255, m[2]);
    }
}

std::vector<float> histogram(const Framebuffer& fb, int bins) {
    std::vector<float> h(size_t(bins), 0.f);
    for (int i = 0, n = fb.width() * fb.height(); i < n; i += 4) {
        uint32_t p = fb.data()[i];
        int y = int(float((p >> 16) & 255) * 0.299f + float((p >> 8) & 255) * 0.587f + float(p & 255) * 0.114f);
        h[size_t(std::min(bins - 1, y * bins / 256))] += 1.f;
    }
    float mx = 1.f;
    for (float v : h) mx = std::max(mx, v);
    for (float& v : h) v /= mx;
    return h;
}

bool SimulatedCamera::flashWouldFire(const Effective& e) const {
    int ev = sceneEv100(sceneFor(e));
    return e.flash == 1 || e.flash == 2 || (e.flash == 0 && ev <= 9);
}

ExposureInfo SimulatedCamera::exposure(const Effective& e) const {
    ExposureModel m = modelExposure(sceneEv100(sceneFor(e)), e);
    ExposureInfo x;
    x.valid = true; x.iso = m.iso; x.shutterSeconds = m.t; x.fNumber = m.f; x.shutter = shutterText(m.t);
    char b[8]; std::snprintf(b, sizeof b, "F%.1f", double(m.f)); x.aperture = b;
    return x;
}

bool SimulatedCamera::liveFrame(Framebuffer& out, const Effective& e, uint64_t timeMs) {
    // Render a slightly larger scene, then crop with a slow drift and digital zoom (3x optical zoom is imitated).
    static Framebuffer src(360, 270);
    static Scene cachedScene = Scene::Flower;
    static bool have = false;
    Scene sc = sceneFor(e);
    if (!have || sc != cachedScene) {
        Framebuffer base;                       // scenes are drawn for 320x240, then enlarged so the preview can drift and zoom
        renderScene(base, sc, 0);
        Canvas(src).blit(base, {0, 0, 320, 240}, {0, 0, 360, 270});
        cachedScene = sc; have = true;
    }
    float z = 1.f + float(e.zoom) / 25.f * 2.f, sw = 320.f / z, sh = 240.f / z;
    float dx = std::sin(float(timeMs) / 2600.f) * 7.f, dy = std::cos(float(timeMs) / 3100.f) * 5.f;
    float sx = std::clamp((360.f - sw) / 2.f + dx, 0.f, 360.f - sw), sy = std::clamp((270.f - sh) / 2.f + dy, 0.f, 270.f - sh);
    Canvas cv(out);
    cv.blit(src, {int(sx), int(sy), int(sw), int(sh)}, {0, 0, out.width(), out.height()});
    applyLook(out, e, false, ++frame_);
    return true;
}

CaptureResult SimulatedCamera::capture(const Effective& e, int64_t wall) {
    CaptureResult r;
    Scene sc = sceneFor(e);
    ExposureModel m = modelExposure(sceneEv100(sc), e);
    auto fb = std::make_shared<Framebuffer>();
    Framebuffer base;
    renderScene(base, sc, 0);
    float z = 1.f + float(e.zoom) / 25.f * 2.f;
    int cw = int(320.f / z), ch = int(240.f / z);
    Canvas(*fb).blit(base, {(320 - cw) / 2, (240 - ch) / 2, cw, ch}, {0, 0, 320, 240});
    r.flashFired = flashWouldFire(e);
    applyLook(*fb, e, r.flashFired, uint32_t(wall));
    r.image = fb;
    r.meta.iso = m.iso; r.meta.shutter = shutterText(m.t);
    char b[8]; std::snprintf(b, sizeof b, "F%.1f", double(m.f)); r.meta.aperture = b;
    r.meta.focalMM = int(35.f + float(e.zoom) / 25.f * 70.f); r.meta.evIdx = e.ev; r.meta.wb = e.wb; r.meta.flash = r.flashFired;
    r.status = Result::success();
    return r;
}

}  // namespace s650
