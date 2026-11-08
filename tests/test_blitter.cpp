#include "display/blitter.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace mc;
static void reference(std::vector<uint16_t> &out, int W, int H, const RasterView &s, int cx, int cy,
                      float zoom) {
    int w = std::max(1, int(s.width * zoom)), h = std::max(1, int(s.height * zoom)),
        left = cx - int(s.anchorX * zoom), top = cy - int(s.anchorY * zoom);
    for (int y = std::max(0, -top); y < std::min(h, H - top); y++)
        for (int x = std::max(0, -left); x < std::min(w, W - left); x++) {
            int sy = std::min(int(s.height) - 1, int(y / zoom)),
                sx = std::min(int(s.width) - 1, int(x / zoom));
            auto p = s.rgba565 + (sy * s.width + sx) * 3;
            if (p[2] > 127)
                out[(top + y) * W + left + x] = uint16_t(p[0]) | uint16_t(p[1]) << 8;
        }
}
int main() {
    for (auto dims : std::array<std::array<int, 2>, 3>{{{{64, 96}}, {{128, 192}}, {{17, 29}}}}) {
        int w = dims[0], h = dims[1];
        std::vector<uint8_t> raw(w * h * 3);
        std::vector<uint16_t> pixels(w * h);
        std::vector<RowBounds> rows(h);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                int i = y * w + x;
                raw[i * 3] = uint8_t(i * 17);
                raw[i * 3 + 1] = uint8_t(i * 23);
                raw[i * 3 + 2] = (x > 3 && x < w - 4 && y > h / 3 && (x + y) % 7)
                                     ? uint8_t((x + y) % 3 == 0 ? 127 : 255)
                                     : 0;
            }
        prepareRaster(raw.data(), pixels.data(), rows.data(), w, h);
        RasterView view{raw.data(),  pixels.data(),   rows.data(),     uint16_t(w),
                        uint16_t(h), uint16_t(w / 2), uint16_t(h - 17)};
        for (int scale = 2; scale <= 6; scale++)
            for (int cx : {-120, -3, 0, 45, 100, 200})
                for (int cy : {-100, 0, 5, 77, 140, 250}) {
                    std::vector<uint16_t> want(117 * 101, 0x1234), got = want;
                    reference(want, 117, 101, view, cx, cy, float(scale) / 4);
                    Blitter blit;
                    blit.setScale(scale);
                    blit.draw(got.data(), 117, 101, view, cx, cy);
                    assert(got == want);
                }
    }
    std::cout << "Blitter: exact reference pixels at five zoom levels, transparency, holes, "
                 "clipping and max-size sprites: PASS\n";
}
