#pragma once
// Text helpers for option values. Kept apart from Options.h so the option tables stay free of the C++ standard library.
#include <string>
namespace s650 {
std::string evLabel(int idx);                // "+0.7", "0.0", "-1.3"
}
