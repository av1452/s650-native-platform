#include "display/Framebuffer.h"
#include <algorithm>
#include <cmath>

namespace s650 {

void Framebuffer::scaleLuma(float f) {
    for (auto& p : px_) {
        int r = int(float((p >> 16) & 255) * f), g = int(float((p >> 8) & 255) * f), b = int(float(p & 255) * f);
        p = 0xFF000000u | (uint32_t(std::min(r, 255)) << 16) | (uint32_t(std::min(g, 255)) << 8) | uint32_t(std::min(b, 255));
    }
}

uint64_t Framebuffer::hash() const {
    uint64_t h = 1469598103934665603ull;
    for (uint32_t p : px_) { h ^= p; h *= 1099511628211ull; }
    return h;
}

void Canvas::pushClip(Rect r) {
    clips_.push_back(clip_);
    int x0 = std::max(clip_.x, r.x), y0 = std::max(clip_.y, r.y);
    int x1 = std::min(clip_.x + clip_.w, r.x + r.w), y1 = std::min(clip_.y + clip_.h, r.y + r.h);
    clip_ = Rect{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}
void Canvas::popClip() { if (!clips_.empty()) { clip_ = clips_.back(); clips_.pop_back(); } }

void Canvas::blendPixel(int x, int y, Color c, int a) {
    if (!clip_.contains(x, y) || a <= 0) return;
    if (a >= 255) { fb_.set(x, y, Framebuffer::pack(c)); return; }
    uint32_t d = fb_.get(x, y);
    int dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
    int r = (c.r * a + dr * (255 - a) + 127) / 255, g = (c.g * a + dg * (255 - a) + 127) / 255, b = (c.b * a + db * (255 - a) + 127) / 255;
    fb_.set(x, y, 0xFF000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b));
}

void Canvas::fillRect(Rect r, Color c, int alpha) {
    int x0 = std::max(r.x, clip_.x), y0 = std::max(r.y, clip_.y);
    int x1 = std::min(r.x + r.w, clip_.x + clip_.w), y1 = std::min(r.y + r.h, clip_.y + clip_.h);
    for (int y = y0; y < y1; ++y) for (int x = x0; x < x1; ++x) blendPixel(x, y, c, alpha);
}

void Canvas::strokeRect(Rect r, Color c, int t, int alpha) {
    fillRect({r.x, r.y, r.w, t}, c, alpha);
    fillRect({r.x, r.y + r.h - t, r.w, t}, c, alpha);
    fillRect({r.x, r.y + t, t, r.h - 2 * t}, c, alpha);
    fillRect({r.x + r.w - t, r.y + t, t, r.h - 2 * t}, c, alpha);
}

void Canvas::gradientV(Rect r, Color a, Color b) {
    for (int y = 0; y < r.h; ++y) {
        float t = r.h > 1 ? float(y) / float(r.h - 1) : 0.f;
        Color c(uint8_t(a.r + (b.r - a.r) * t), uint8_t(a.g + (b.g - a.g) * t), uint8_t(a.b + (b.b - a.b) * t));
        fillRect({r.x, r.y + y, r.w, 1}, c);
    }
}

static float segDist(float px, float py, float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
    float t = l2 > 0 ? std::clamp(((px - x0) * dx + (py - y0) * dy) / l2, 0.f, 1.f) : 0.f;
    float qx = x0 + t * dx - px, qy = y0 + t * dy - py;
    return std::sqrt(qx * qx + qy * qy);
}

void Canvas::line(float x0, float y0, float x1, float y1, float th, Color c, int alpha) {
    float half = th * 0.5f;
    int ix0 = int(std::floor(std::min(x0, x1) - half - 1)), ix1 = int(std::ceil(std::max(x0, x1) + half + 1));
    int iy0 = int(std::floor(std::min(y0, y1) - half - 1)), iy1 = int(std::ceil(std::max(y0, y1) + half + 1));
    for (int y = iy0; y <= iy1; ++y) for (int x = ix0; x <= ix1; ++x) {
        float d = segDist(float(x) + 0.5f, float(y) + 0.5f, x0, y0, x1, y1);
        float cov = std::clamp(half + 0.5f - d, 0.f, 1.f);
        if (cov > 0) blendPixel(x, y, c, int(cov * float(alpha)));
    }
}

void Canvas::circle(float cx, float cy, float r, float th, Color c, int alpha) {
    float half = th * 0.5f;
    for (int y = int(cy - r - th - 1); y <= int(cy + r + th + 1); ++y)
        for (int x = int(cx - r - th - 1); x <= int(cx + r + th + 1); ++x) {
            float d = std::fabs(std::hypot(float(x) + 0.5f - cx, float(y) + 0.5f - cy) - r);
            float cov = std::clamp(half + 0.5f - d, 0.f, 1.f);
            if (cov > 0) blendPixel(x, y, c, int(cov * float(alpha)));
        }
}

void Canvas::fillCircle(float cx, float cy, float r, Color c, int alpha) {
    for (int y = int(cy - r - 1); y <= int(cy + r + 1); ++y)
        for (int x = int(cx - r - 1); x <= int(cx + r + 1); ++x) {
            float d = std::hypot(float(x) + 0.5f - cx, float(y) + 0.5f - cy);
            float cov = std::clamp(r + 0.5f - d, 0.f, 1.f);
            if (cov > 0) blendPixel(x, y, c, int(cov * float(alpha)));
        }
}

void Canvas::fillPoly(const std::vector<Pt>& p, Color c, int alpha) {
    if (p.size() < 3) return;
    float ymin = p[0].second, ymax = p[0].second;
    for (auto& q : p) { ymin = std::min(ymin, q.second); ymax = std::max(ymax, q.second); }
    for (int y = int(std::floor(ymin)); y <= int(std::ceil(ymax)); ++y) {
        float sy = float(y) + 0.5f;
        std::vector<float> xs;
        for (size_t i = 0; i < p.size(); ++i) {
            const Pt& a = p[i]; const Pt& b = p[(i + 1) % p.size()];
            if ((a.second <= sy && b.second > sy) || (b.second <= sy && a.second > sy))
                xs.push_back(a.first + (sy - a.second) / (b.second - a.second) * (b.first - a.first));
        }
        std::sort(xs.begin(), xs.end());
        for (size_t i = 0; i + 1 < xs.size(); i += 2)
            for (int x = int(std::floor(xs[i] + 0.5f)); x < int(std::floor(xs[i + 1] + 0.5f)); ++x) blendPixel(x, y, c, alpha);
    }
}

void Canvas::polyline(const std::vector<Pt>& p, bool closed, float th, Color c, int alpha) {
    for (size_t i = 0; i + 1 < p.size(); ++i) line(p[i].first, p[i].second, p[i + 1].first, p[i + 1].second, th, c, alpha);
    if (closed && p.size() > 2) line(p.back().first, p.back().second, p[0].first, p[0].second, th, c, alpha);
}

void Canvas::blit(const Framebuffer& src, Rect s, Rect d, int alpha) {
    if (d.w <= 0 || d.h <= 0 || s.w <= 0 || s.h <= 0) return;
    for (int y = 0; y < d.h; ++y) {
        int dy = d.y + y;
        for (int x = 0; x < d.w; ++x) {
            int dx = d.x + x;
            if (!clip_.contains(dx, dy)) continue;
            float fx = s.x + (float(x) + 0.5f) * float(s.w) / float(d.w) - 0.5f;
            float fy = s.y + (float(y) + 0.5f) * float(s.h) / float(d.h) - 0.5f;
            int x0 = std::clamp(int(std::floor(fx)), 0, src.width() - 1), y0 = std::clamp(int(std::floor(fy)), 0, src.height() - 1);
            int x1 = std::min(x0 + 1, src.width() - 1), y1 = std::min(y0 + 1, src.height() - 1);
            float tx = std::clamp(fx - float(x0), 0.f, 1.f), ty = std::clamp(fy - float(y0), 0.f, 1.f);
            float acc[3] = {0, 0, 0};
            const uint32_t px[4] = {src.get(x0, y0), src.get(x1, y0), src.get(x0, y1), src.get(x1, y1)};
            const float w[4] = {(1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty};
            for (int i = 0; i < 4; ++i) { acc[0] += float((px[i] >> 16) & 255) * w[i]; acc[1] += float((px[i] >> 8) & 255) * w[i]; acc[2] += float(px[i] & 255) * w[i]; }
            blendPixel(dx, dy, Color(uint8_t(acc[0] + 0.5f), uint8_t(acc[1] + 0.5f), uint8_t(acc[2] + 0.5f)), alpha);
        }
    }
}

}  // namespace s650
