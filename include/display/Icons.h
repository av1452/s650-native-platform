#pragma once
// Vector line icons on the same 24-unit grid as the browser prototype (js/icons.js), drawn with anti-aliased strokes.
// Shapes were re-drawn by hand for this project; they are simplified compared with the SVG originals.
#include "display/Framebuffer.h"

namespace s650 {
enum class Icon { Camera, Size, Focus, Flash, Exposure, WhiteBalance, Iso, Setup, Timer, Macro, FlashOff, Card, Lock, Images };
// Draws the icon inside the square (x, y, size, size).
void drawIcon(Canvas& cv, Icon icon, int x, int y, int size, Color color, int alpha = 255);
void drawBattery(Canvas& cv, int x, int y, int w, int h, int percent, Color color);
Icon categoryIcon(int category);   // 0..7 in menu order
}  // namespace s650
