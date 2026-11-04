#include "core/Strings.h"

namespace s650 {

std::string tr(const char* key) {
    return std::string(translate(key));
}

std::string tr(const std::string& key) {
    return std::string(translate(key.c_str()));
}

} // namespace s650
