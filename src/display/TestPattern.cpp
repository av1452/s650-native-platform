#include "display/TestPattern.h"

namespace s650 {

void drawTestPattern(Canvas& cv) {
    cv.fillRect({0, 0, kLcdW, kLcdH}, Color(0, 0, 0));
    static const Color bars[8] = {{255, 255, 255}, {255, 255, 0}, {0, 255, 255}, {0, 255, 0}, {255, 0, 255}, {255, 0, 0}, {0, 0, 255}, {16, 16, 16}};
    for (int i = 0; i < 8; ++i) cv.fillRect({i * 40, 12, 40, 96}, bars[i]);
    for (int i = 0; i < 16; ++i) cv.fillRect({i * 20, 112, 20, 36}, Color(uint8_t(i * 17), uint8_t(i * 17), uint8_t(i * 17)));
    cv.strokeRect({0, 0, kLcdW, kLcdH}, Color(255, 255, 255), 1);
    cv.fillRect({1, 1, 8, 8}, Color(255, 0, 0));            cv.fillRect({kLcdW - 9, 1, 8, 8}, Color(0, 255, 0));
    cv.fillRect({1, kLcdH - 9, 8, 8}, Color(0, 0, 255));    cv.fillRect({kLcdW - 9, kLcdH - 9, 8, 8}, Color(255, 255, 0));
    cv.fillRect({0, 120 - 1, kLcdW, 1}, Color(128, 128, 128));       // crosshair through the middle of the frame
    cv.fillRect({kLcdW / 2, 0, 1, kLcdH}, Color(128, 128, 128));
    cv.circle(80.f, 190.f, 36.f, 2.f, Color(255, 255, 255));        // a round circle on square pixels; an ellipse if pixels are not square
    TextStyle st; st.color = Color(255, 255, 255); st.scale = 2;
    cv.text(140, 168, "S650 TEST", st);
    st.scale = 1; st.color = Color(160, 160, 160);
    cv.text(140, 192, "320x240 LOGICAL", st);
    cv.text(140, 204, "NOT CAMERA OUTPUT", st);
}

}  // namespace s650
