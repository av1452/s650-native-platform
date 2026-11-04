#pragma once
// The three appearances of Version 2.0 (values copied from css/tokens.css and S650-INTERFACE-PORTING-SPEC.md).
#include "core/Types.h"

namespace s650 {
struct Palette {
    Color bg, bg2, panel, panel2, head, ink, dim, faint, line, accent, accentInk, ok, warn;
    bool classic;          // classic uses a solid accent selection bar and top tabs
};
const Palette& palette(int style);   // 0 modern dark, 1 light, 2 classic blue

// Fixed structure (logical pixels)
constexpr int kHdrH = 22, kHintH = 16, kDescH = 34, kRowH = 24, kRailW = 38, kClassicTabsH = 30;
constexpr int kBodyTop = kHdrH, kBodyBottom = kLcdH - kDescH - kHintH;      // 22 .. 190 (modern: 7 rows)
}  // namespace s650
