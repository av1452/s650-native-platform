#include "core/Translation.h"

namespace s650 {
namespace {

bool equal(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) return false;

    while (*a != '\0' && *b != '\0') {
        if (*a != *b) return false;
        ++a;
        ++b;
    }

    return *a == *b;
}

} // namespace

const char* translate(const char* key) noexcept {
    if (key == nullptr) return "";

    for (size_t i = 0; i < kEnglishCount; ++i) {
        if (equal(key, kEnglish[i].key)) {
            return kEnglish[i].value;
        }
    }

    return key;
}

} // namespace s650
