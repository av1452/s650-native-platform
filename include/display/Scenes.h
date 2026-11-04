#pragma once
// Procedural pictures that stand in for the live view and for stored photographs in the SIMULATED backends.
// Real hardware would supply sensor frames and JPEG files; none of this code is meant to run on a camera.
#include "display/Framebuffer.h"
namespace s650 {
enum class Scene { Lake, Sunset, City, Flower, Forest, Snow, Dunes, Harbor };
void renderScene(Framebuffer& fb, Scene scene, int variant = 0);
int sceneEv100(Scene scene);     // simulated brightness (EV at ISO 100)
}  // namespace s650
