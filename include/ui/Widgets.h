#pragma once
// Shared drawing routines for header, hint bar, description bar, list rows, dialogs. All geometry follows
// S650-INTERFACE-PORTING-SPEC.md section 3.
#include <functional>
#include <string>
#include <vector>
#include "display/Framebuffer.h"
#include "ui/Theme.h"

namespace s650 {

struct Hint { std::string key; std::string labelKey; };

TextStyle style(const Color& c, bool bold = false, int scale = 1, int alpha = 255);

void drawHeader(Canvas& cv, const Palette& p, int y, const std::string& crumb, const std::string& title, const std::string& meta);
void drawHintBar(Canvas& cv, const Palette& p, const std::vector<Hint>& hints, bool translucent = false);
void drawDescBar(Canvas& cv, const Palette& p, const std::string& name, const std::string& text, bool warn);
std::vector<std::string> wrapText(const std::string& text, int maxWidth, int scale = 1);
void drawScrollbar(Canvas& cv, const Palette& p, Rect area, int total, int visible, int top);

struct RowView {
    std::string label, value;
    bool selected = false, disabled = false, danger = false, chevron = false;
};
void drawRow(Canvas& cv, const Palette& p, Rect r, const RowView& v);

struct DialogView { std::string title, message, ok, cancel; bool danger = false, focusOk = false; };
void drawDialog(Canvas& cv, const Palette& p, const DialogView& d);

}  // namespace s650
