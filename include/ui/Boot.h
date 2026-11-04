#pragma once
// Startup, shutdown and "off" frames (timeline from S650-INTERFACE-PORTING-SPEC.md section 12).
#include "display/Framebuffer.h"
namespace s650 {
void renderBoot(Canvas& cv, uint32_t elapsedMs);          // 0..1900 ms: wordmark, accent line, model caption
void renderShutdownLogo(Canvas& cv, uint32_t elapsedMs);  // wordmark visible then fading out (elapsed counts from shutdown start)
void renderOff(Canvas& cv);
}  // namespace s650
