#include "input/InputBackend.h"
#include <cctype>

namespace s650 {
// keyCode is a platform-neutral code supplied by the window backend: 1000+ for arrows/specials, else 0 and ch is used.
// 1000 up, 1001 down, 1002 left, 1003 right, 1004 enter, 1005 escape, 1006 delete/backspace
Action actionForKey(int code, char ch) {
    switch (code) {
        case 1000: return Action::Up; case 1001: return Action::Down; case 1002: return Action::Left; case 1003: return Action::Right;
        case 1004: return Action::Ok; case 1005: return Action::Back; case 1006: return Action::Delete; default: break;
    }
    switch (std::tolower(static_cast<unsigned char>(ch))) {
        case 'm': return Action::Menu; case 'p': return Action::Play; case 'd': return Action::Disp;
        case 'w': case '-': return Action::Wide; case 't': case '+': case '=': return Action::Tele;
        case ' ': return Action::Shutter; case '[': return Action::ModePrev; case ']': return Action::ModeNext;
        case 'o': return Action::Power; case 'k': return Action::Protect; default: return Action::None;
    }
}
}  // namespace s650
