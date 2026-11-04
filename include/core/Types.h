#pragma once
// Basic value types shared by every module. No platform dependencies.
#include <stdint.h>   // C header only: this file must stay free of the C++ standard library (see docs/EMBEDDED-PORTABILITY.md)

namespace s650 {

struct Color {
    uint8_t r = 0, g = 0, b = 0;
    constexpr Color() = default;
    constexpr Color(uint8_t R, uint8_t G, uint8_t B) : r(R), g(G), b(B) {}
};
constexpr Color rgb(uint32_t hex) { return Color(uint8_t(hex >> 16), uint8_t(hex >> 8), uint8_t(hex)); }

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    constexpr bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};

// Logical camera buttons. These are the ONLY input the interface understands. Which physical
// switch produces which action is the InputBackend's job (unknown for the real S650).
enum class Action {
    None, Up, Down, Left, Right, Ok, Back, Menu, Play, Wide, Tele,
    Shutter,            // Press = half-press, Release = full release (shoot)
    ModePrev, ModeNext, Power, Delete, Protect, Disp
};
enum class Phase { Press, Release };

struct InputEvent {
    Action action = Action::None;
    Phase phase = Phase::Press;
};

constexpr int kLcdW = 320;   // logical display assumption (NOT verified for the real panel)
constexpr int kLcdH = 240;

}  // namespace s650
