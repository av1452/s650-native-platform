#include "display/Scenes.h"
#include <cmath>

namespace s650 {
namespace {
struct Rng { uint32_t s; float next() { s = s * 1664525u + 1013904223u; return float(s >> 8) / 16777216.f; } };

void mountains(Canvas& cv, float base, float h, Color c, Rng& r, int peaks) {
    std::vector<Pt> p{{0, 240}, {0, base}};
    for (int i = 0; i <= peaks; ++i) p.push_back({float(i) * 320.f / float(peaks), base - h * (0.4f + 0.6f * r.next()) * ((i % 2) ? 1.f : 0.5f)});
    p.push_back({320, base}); p.push_back({320, 240});
    cv.fillPoly(p, c);
}
}  // namespace

int sceneEv100(Scene s) {
    static const int ev[8] = {14, 12, 7, 12, 11, 15, 15, 14};
    return ev[int(s)];
}

void renderScene(Framebuffer& fb, Scene sc, int variant) {
    Canvas cv(fb);
    Rng r{uint32_t(1234 + int(sc) * 77 + variant * 13)};
    switch (sc) {
    case Scene::Lake: {
        cv.gradientV({0, 0, 320, 130}, rgb(0x5d9ad8), rgb(0xcfe4f5));
        cv.fillCircle(210 + float(variant % 3) * 15, 36, 14, rgb(0xfff6d2));
        mountains(cv, 130, 60, rgb(0x6f8fb4), r, 6);
        mountains(cv, 135, 40, rgb(0x41607f), r, 9);
        cv.gradientV({0, 135, 320, 105}, rgb(0x3f6a92), rgb(0x8fc0de));
        break; }
    case Scene::Sunset:
        cv.gradientV({0, 0, 320, 150}, rgb(0x2b2f6b), rgb(0xff9a5a));
        cv.fillCircle(160, 130, 26, rgb(0xffd98a));
        cv.gradientV({0, 150, 320, 90}, rgb(0x6b3d5a), rgb(0x1d1a30));
        for (int i = 0; i < 5; ++i) cv.fillRect({90 + i * 6, 155 + i * 9, 140 - i * 12, 2}, rgb(0xffb070), 160);
        break;
    case Scene::City:
        cv.gradientV({0, 0, 320, 240}, rgb(0x0a0f2c), rgb(0x2a2f5c));
        for (int i = 0; i < 12; ++i) {
            int w = 18 + int(r.next() * 14), h = 50 + int(r.next() * 110), x = i * 27;
            cv.fillRect({x, 240 - h, w, h}, rgb(0x14183a));
            for (int wy = 240 - h + 6; wy < 232; wy += 9) for (int wx = x + 3; wx < x + w - 3; wx += 6)
                if (r.next() > 0.45f) cv.fillRect({wx, wy, 3, 4}, rgb(0xffd877));
        }
        break;
    case Scene::Flower:
        cv.gradientV({0, 0, 320, 240}, rgb(0x2f6b3a), rgb(0x7fb25a));
        for (int k = 0; k < 8; ++k) {
            float a = float(k) * 0.7853982f;
            cv.fillCircle(160 + 38 * std::cos(a), 105 + 38 * std::sin(a), 24, variant % 2 ? rgb(0xf5d33a) : rgb(0xe95fa0));
        }
        cv.fillCircle(160, 105, 18, variant % 2 ? rgb(0x7a4a14) : rgb(0xf7e07a));
        cv.fillRect({157, 150, 6, 90}, rgb(0x2a5a2a));
        break;
    case Scene::Forest:
        cv.gradientV({0, 0, 320, 240}, rgb(0xbfe0c0), rgb(0x4f8f55));
        cv.fillPoly({{120, 240}, {200, 240}, {175, 130}, {145, 130}}, rgb(0xd8c58a));
        for (int i = 0; i < 9; ++i) { int x = 10 + i * 36; cv.fillRect({x + 8, 120, 6, 40}, rgb(0x5a4026)); cv.fillCircle(float(x) + 11, 110, 20, rgb(0x2f7a3a)); }
        break;
    case Scene::Snow:
        cv.gradientV({0, 0, 320, 240}, rgb(0xd7e8f5), rgb(0xffffff));
        mountains(cv, 150, 70, rgb(0xb8cde0), r, 5);
        for (int i = 0; i < 6; ++i) cv.fillCircle(30 + float(i) * 50, 190 + r.next() * 25, 10, rgb(0x3f6f4a));
        break;
    case Scene::Dunes:
        cv.gradientV({0, 0, 320, 240}, rgb(0xf4c27a), rgb(0xd88a3a));
        cv.fillCircle(240, 44, 16, rgb(0xfff0b8));
        mountains(cv, 170, 50, rgb(0xe0a050), r, 4);
        mountains(cv, 210, 40, rgb(0xc98a3a), r, 3);
        break;
    case Scene::Harbor:
        cv.gradientV({0, 0, 320, 130}, rgb(0x6aa6d8), rgb(0xcfe4f5));
        mountains(cv, 120, 55, rgb(0x6f8fb4), r, 5);
        cv.gradientV({0, 130, 320, 110}, rgb(0x2f78a8), rgb(0x57a0c8));
        cv.fillRect({0, 195, 320, 12}, rgb(0x7a5a38));
        for (int i = 0; i < 3; ++i) {
            float x = 50.f + float(i) * 95.f;
            cv.fillPoly({{x, 130}, {x + 22, 168}, {x, 168}}, i == 0 ? rgb(0xe0503a) : rgb(0xf4f4f4));
            cv.fillRect({int(x) - 18, 168, 46, 7}, rgb(0x2a2a30));
        }
        break;
    }
}
}  // namespace s650
