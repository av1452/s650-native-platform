#include "core/OptionsText.h"
#include <cstdio>
#include "core/Options.h"

namespace s650 {
std::string evLabel(int idx) {
    float v = evValue(idx);
    char b[16];
    if (idx == 6) return "0.0";
    std::snprintf(b, sizeof b, "%s%.1f", v > 0 ? "+" : "", double(v));
    return b;
}
}  // namespace s650
