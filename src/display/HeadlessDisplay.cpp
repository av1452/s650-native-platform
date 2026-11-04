#include "display/DisplayBackend.h"
#include "display/Png.h"
namespace s650 { bool HeadlessDisplay::savePng(const std::string& path, int scale) const { return writePng(path, last_, scale); } }
