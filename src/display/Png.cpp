#include "display/Png.h"
#include <cstdio>
#include <vector>

namespace s650 {
namespace {
uint32_t crcTable[256];
bool crcInit = [] {
    for (uint32_t n = 0; n < 256; ++n) { uint32_t c = n; for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; crcTable[n] = c; }
    return true;
}();
uint32_t crc(const uint8_t* d, size_t n, uint32_t c = 0xFFFFFFFFu) { for (size_t i = 0; i < n; ++i) c = crcTable[(c ^ d[i]) & 255] ^ (c >> 8); return c; }
void be32(std::vector<uint8_t>& v, uint32_t x) { v.push_back(uint8_t(x >> 24)); v.push_back(uint8_t(x >> 16)); v.push_back(uint8_t(x >> 8)); v.push_back(uint8_t(x)); }
void chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
    be32(out, uint32_t(data.size()));
    std::vector<uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    out.insert(out.end(), td.begin(), td.end());
    be32(out, crc(td.data(), td.size()) ^ 0xFFFFFFFFu);
}
}  // namespace

bool writePng(const std::string& path, const Framebuffer& fb, int scale) {
    (void)crcInit;
    int W = fb.width() * scale, H = fb.height() * scale;
    std::vector<uint8_t> raw;
    raw.reserve(size_t(H) * (size_t(W) * 3 + 1));
    for (int y = 0; y < H; ++y) {
        raw.push_back(0);
        for (int x = 0; x < W; ++x) {
            uint32_t p = fb.get(x / scale, y / scale);
            raw.push_back(uint8_t(p >> 16)); raw.push_back(uint8_t(p >> 8)); raw.push_back(uint8_t(p));
        }
    }
    std::vector<uint8_t> z{0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (uint8_t v : raw) { a = (a + v) % 65521; b = (b + a) % 65521; }
    for (size_t pos = 0; pos < raw.size();) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        z.push_back(pos + n >= raw.size() ? 1 : 0);
        z.push_back(uint8_t(n)); z.push_back(uint8_t(n >> 8)); z.push_back(uint8_t(~n)); z.push_back(uint8_t((~n) >> 8));
        z.insert(z.end(), raw.begin() + long(pos), raw.begin() + long(pos + n));
        pos += n;
    }
    be32(z, (b << 16) | a);
    std::vector<uint8_t> out{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}, ihdr;
    be32(ihdr, uint32_t(W)); be32(ihdr, uint32_t(H));
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
    chunk(out, "IHDR", ihdr); chunk(out, "IDAT", z); chunk(out, "IEND", {});
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite(out.data(), 1, out.size(), f) == out.size();
    std::fclose(f);
    return ok;
}
}  // namespace s650
