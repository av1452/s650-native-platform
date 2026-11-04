#pragma once
// Minimal PNG writer (uncompressed "stored" deflate blocks) so screenshots need no image library.
#include <string>
#include "display/Framebuffer.h"
namespace s650 {
bool writePng(const std::string& path, const Framebuffer& fb, int scale = 1);   // scale: integer nearest-neighbour enlargement
}
