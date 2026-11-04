#pragma once
// Display test pattern for the first LCD milestone (docs/LCD-RENDERING-RESEARCH.md): 8 colour bars, a 16-step grey ramp, a border,
// coloured corner squares, a centre cross, a circle (reveals non-square pixels) and a text label. Defined on the 320x240 logical frame
// so the identical pattern can be produced on the desktop and, in future, on any display backend.
#include "display/Framebuffer.h"
namespace s650 {
void drawTestPattern(Canvas& cv);
}
