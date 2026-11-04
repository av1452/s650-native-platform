#include "core/DateTime.h"
#include <cstdio>
#include <ctime>

namespace s650 {

void splitDateTime(int64_t s, int& y, int& mo, int& d, int& h, int& mi) {
    std::time_t t = std::time_t(s);
    std::tm tm{};
    gmtime_r(&t, &tm);
    y = tm.tm_year + 1900; mo = tm.tm_mon + 1; d = tm.tm_mday; h = tm.tm_hour; mi = tm.tm_min;
}

int64_t joinDateTime(int y, int mo, int d, int h, int mi) {
    std::tm tm{};
    tm.tm_year = y - 1900; tm.tm_mon = mo - 1; tm.tm_mday = d; tm.tm_hour = h; tm.tm_min = mi;
    return int64_t(timegm(&tm));   // normalises e.g. Feb 31 to Mar 3
}

std::string formatDate(int64_t s) {
    int y, mo, d, h, mi; splitDateTime(s, y, mo, d, h, mi);
    char b[16]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", y, mo, d); return b;
}
std::string formatTime(int64_t s) {
    int y, mo, d, h, mi; splitDateTime(s, y, mo, d, h, mi);
    char b[8]; std::snprintf(b, sizeof b, "%02d:%02d", h, mi); return b;
}
std::string formatDateTime(int64_t s) { return formatDate(s) + " " + formatTime(s); }

}  // namespace s650
