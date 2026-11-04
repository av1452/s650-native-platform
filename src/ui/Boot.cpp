#include "ui/Boot.h"
#include <algorithm>
#include "core/Strings.h"
#include "ui/Widgets.h"

namespace s650 {
namespace {
float ramp(uint32_t t, uint32_t start, uint32_t dur) { return t <= start ? 0.f : std::min(1.f, float(t - start) / float(dur)); }
float ease(float x) { return 1.f - (1.f - x) * (1.f - x); }

void drawLogo(Canvas& cv, float aBrand, float aShot, float aRule, float fill, float aModel, int rise) {
    cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0));
    // "SONY" with wide letter-spacing (generic stand-in, not the real logo artwork)
    const char* word = "SONY";
    int total = 4 * 12 + 3 * 9, x = (kLcdW - total) / 2 + 3;
    for (int i = 0; i < 4; ++i) { TextStyle s = style(Color(255, 255, 255), true, 2, int(aBrand * 255)); cv.text(x + i * 21, 84 + rise, std::string(1, word[i]), s); }
    cv.textCenter(160, 108, "Cyber-shot", style(Color(255, 255, 255), true, 3, int(aShot * 255)));
    cv.fillRect({85, 142, 150, 2}, Color(255, 255, 255), int(aRule * 36));
    cv.fillRect({85, 142, int(150.f * fill), 2}, Color(0xf2, 0x8a, 0x1b), int(aRule * 255));
    cv.textCenter(160, 154, tr("boot.model"), style(Color(255, 255, 255), false, 1, int(aModel * 128)));
}
}  // namespace

void renderBoot(Canvas& cv, uint32_t t) {
    float brand = ease(ramp(t, 60, 520)), shot = ease(ramp(t, 200, 520)), rule = ramp(t, 60, 300), fill = ease(ramp(t, 620, 760)), model = ramp(t, 260, 400);
    drawLogo(cv, brand, shot, rule, fill, model, int((1.f - brand) * 2.f));
}

void renderShutdownLogo(Canvas& cv, uint32_t t) {
    float fade = 1.f - ramp(t, 900, 360);     // wordmark fades out after 900 ms
    drawLogo(cv, fade, fade, fade, 1.f, fade, 0);
}

void renderOff(Canvas& cv) {
    cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0));
    cv.textCenter(160, 226, tr("off.msg"), style(Color(255, 255, 255), false, 1, 72));
}

}  // namespace s650
