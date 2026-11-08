#pragma once
#include <array>
#include <cstdint>
namespace mc {
struct RowBounds {
    uint16_t first = 0, last = 0;
};
struct RasterView {
    const uint8_t *rgba565 = nullptr;
    const uint16_t *colors = nullptr;
    const RowBounds *rows = nullptr;
    uint16_t width = 0, height = 0, anchorX = 0, anchorY = 0;
};
// Nearest-neighbor zoom in quarters. No floating point or SD access in pixel loops.
class Blitter {
    int scale = 4;
    std::array<uint16_t, 384> sourceX{};
    std::array<uint16_t, 480> sourceY{};

  public:
    Blitter() {
        setScale(4);
    }
    void setScale(int quarters);
    void draw(uint16_t *target, int width, int height, const RasterView &, int centerX,
              int centerY) const;
};
void prepareRaster(const uint8_t *raw, uint16_t *colors, RowBounds *rows, int width, int height);
} // namespace mc
