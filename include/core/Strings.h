#pragma once

#include "core/Translation.h"

#include <string>

namespace s650 {

// Desktop compatibility adapter.
// Embedded code should use translate() directly.
std::string tr(const char* key);
std::string tr(const std::string& key);

} // namespace s650
