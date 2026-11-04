#pragma once
// Calendar helpers on "naive wall seconds". Separate from Clock.h so the clock interface needs no standard library.
#include <string>
#include <stdint.h>

namespace s650 {
std::string formatDate(int64_t wallSeconds);   // YYYY-MM-DD
std::string formatTime(int64_t wallSeconds);   // HH:MM
std::string formatDateTime(int64_t wallSeconds);
void splitDateTime(int64_t wallSeconds, int& y, int& mo, int& d, int& h, int& mi);
int64_t joinDateTime(int y, int mo, int d, int h, int mi);

}  // namespace s650
