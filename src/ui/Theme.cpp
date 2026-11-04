#include "ui/Theme.h"
namespace s650 {
const Palette& palette(int style) {
    static const Palette p[3] = {
        {rgb(0x0d0e10), rgb(0x131518), rgb(0x191c20), rgb(0x22262c), rgb(0x08090a), rgb(0xeef0f2), rgb(0x8f979f), rgb(0x5b626a), rgb(0x292d33), rgb(0xf28a1b), rgb(0x0d0e10), rgb(0x58d68d), rgb(0xff7a6b), false},
        {rgb(0xeceef0), rgb(0xe3e6e9), rgb(0xf5f6f7), rgb(0xffffff), rgb(0xd9dde1), rgb(0x16181b), rgb(0x565d64), rgb(0x98a0a7), rgb(0xc7ccd1), rgb(0xd95f00), rgb(0xffffff), rgb(0x1f9d55), rgb(0xc8321f), false},
        {rgb(0x0c2150), rgb(0x0e2a66), rgb(0x12306b), rgb(0x1a3f85), rgb(0x081738), rgb(0xffffff), rgb(0xa3b7de), rgb(0x6f86b8), rgb(0x2a4f9a), rgb(0xff9f1a), rgb(0x101010), rgb(0x7be39b), rgb(0xff8b7d), true}};
    return p[style < 0 ? 0 : style > 2 ? 2 : style];
}
}  // namespace s650
