#pragma once
#include "display/blitter.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
namespace mc {
struct Sprite {
    uint16_t id = 0, w = 0, h = 0, ax = 0, ay = 0, format = 0;
    uint32_t offset = 0, length = 0;
};
struct Cached {
    uint16_t id = 65535;
    uint8_t *data = nullptr;
    uint16_t *colors = nullptr;
    RowBounds *rows = nullptr;
    size_t size = 0;
    uint32_t used = 0;
};
class Assets {
    FILE *file = nullptr;
    std::array<Sprite, 256> catalog{};
    unsigned count = 0;
    std::array<Cached, 128> cache{};
    uint32_t clock = 0;
    std::array<int16_t, 256> catalogIndex{}, cacheIndex{};

  public:
    std::string path, error;
    uint32_t version = 0;
    size_t bytes = 0, hits = 0, misses = 0;
    Assets() {
        catalogIndex.fill(-1);
        cacheIndex.fill(-1);
    }
    RasterView raster(uint16_t) const;
    ~Assets() {
        close();
    }
    void close();
    bool open(const std::string &);
    bool prefetch(uint16_t);
    const Sprite *sprite(uint16_t) const;
    const uint8_t *cached(uint16_t) const;
    static bool validate(const std::string &, uint32_t &version, std::string &error);
};
bool selectAssets(const std::string &root, std::string &path);
bool publishAssets(const std::string &root, int slot, uint32_t sequence);
uint32_t assetSequence(const std::string &root, int slot);
} // namespace mc
