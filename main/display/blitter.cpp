#include "blitter.hpp"
#include <algorithm>
#include <cstring>
namespace mc {
void prepareRaster(const uint8_t *raw, uint16_t *colors, RowBounds *rows, int width, int height) {
    for (int y = 0; y < height; y++) {
        rows[y] = {uint16_t(width), 0};
        for (int x = 0; x < width; x++) {
            int i = y * width + x;
            colors[i] = uint16_t(raw[i * 3]) | uint16_t(raw[i * 3 + 1]) << 8;
            if (raw[i * 3 + 2] >= 128) {
                rows[y].first = std::min(rows[y].first, uint16_t(x));
                rows[y].last = x + 1;
            }
        }
    }
}
void Blitter::setScale(int quarters) {
    scale = std::clamp(quarters, 2, 6);
    for (unsigned x = 0; x < sourceX.size(); x++)
        sourceX[x] = x * 4 / scale;
    for (unsigned y = 0; y < sourceY.size(); y++)
        sourceY[y] = y * 4 / scale;
}
void Blitter::draw(uint16_t *target, int width, int height, const RasterView &s, int cx,
                   int cy) const {
    if (!target || !s.rgba565 || !s.colors || !s.rows || s.width > 256 || s.height > 320)
        return;
    int w = s.width * scale / 4, h = s.height * scale / 4;
    int left = cx - s.anchorX * scale / 4, top = cy - s.anchorY * scale / 4;
    for (int y = std::max(0, -top); y < std::min(h, height - top); y++) {
        int sy = sourceY[y];
        const auto &span = s.rows[sy];
        if (span.first >= span.last)
            continue;
        int start = std::max({0, -left, (int(span.first) * scale + 3) / 4});
        int end = std::min({w, width - left, (int(span.last) * scale + 3) / 4});
        if (start >= end)
            continue;
        auto dest = target + (top + y) * width;
        auto pixels = s.colors + sy * s.width;
        auto raw = s.rgba565 + sy * s.width * 3;
        if (scale == 4) {
            int x = start;
            while (x < end) {
                while (x < end && raw[x * 3 + 2] < 128)
                    x++;
                int begin = x;
                while (x < end && raw[x * 3 + 2] >= 128)
                    x++;
                if (x > begin)
                    memcpy(dest + left + begin, pixels + begin,
                           size_t(x - begin) * sizeof(uint16_t));
            }
        } else
            for (int x = start; x < end; x++) {
                int sx = sourceX[x];
                if (raw[sx * 3 + 2] >= 128)
                    dest[left + x] = pixels[sx];
            }
    }
}
} // namespace mc
