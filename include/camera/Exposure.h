#pragma once
// A toy exposure model and image "look" used only by SimulatedCamera so that settings visibly do something.
// It is a simplification for the desktop simulation, not a model of the real camera's metering or processing.
#include <string>
#include <vector>
#include "core/Settings.h"
#include "display/Framebuffer.h"

namespace s650 {
struct ExposureModel { int iso; float f; float t; };
ExposureModel modelExposure(int sceneEv100, const Effective& eff);
std::string shutterText(float seconds);
void applyLook(Framebuffer& fb, const Effective& eff, bool flashed, uint32_t noiseSeed);
std::vector<float> histogram(const Framebuffer& fb, int bins);   // luminance, normalised 0..1 (also used for the on-screen histogram)
}  // namespace s650
