#pragma once
// Text persistence of Settings. Separate from Settings.h so the settings data itself needs no standard library.
#include <string>
#include "core/Settings.h"

namespace s650 {
std::string serialize(const Settings& s);                    // key=value lines
bool deserialize(const std::string& text, Settings& out);    // false if the text is not recognisable; clamps values

}  // namespace s650
