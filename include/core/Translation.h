#pragma once

#include <stddef.h>

namespace s650 {

struct TranslationEntry {
    const char* key;
    const char* value;
};

extern const TranslationEntry kEnglish[];
extern const size_t kEnglishCount;

// Allocation-free lookup. Returns an empty string for a null key.
// Missing keys are returned unchanged.
const char* translate(const char* key) noexcept;

// Small fixed-capacity translation-key builder.
// N includes space for the terminating null character.
template <size_t N>
class FixedKey {
    static_assert(N > 0, "FixedKey capacity must include a terminator");

public:
    FixedKey(const char* prefix,
             const char* separator,
             const char* suffix) noexcept
        : buffer_{}, length_(0), valid_(true) {
        append(prefix);
        append(separator);
        append(suffix);

        if (!valid_) {
            buffer_[0] = '\0';
            length_ = 0;
        }
    }

    bool valid() const noexcept { return valid_; }
    const char* c_str() const noexcept { return buffer_; }
    size_t size() const noexcept { return length_; }
    static constexpr size_t capacity() noexcept { return N; }

private:
    void append(const char* text) noexcept {
        if (!valid_ || text == nullptr) {
            valid_ = false;
            return;
        }

        while (*text != '\0') {
            if (length_ >= N - 1) {
                valid_ = false;
                return;
            }

            buffer_[length_++] = *text++;
        }

        buffer_[length_] = '\0';
    }

    char buffer_[N];
    size_t length_;
    bool valid_;
};

} // namespace s650
