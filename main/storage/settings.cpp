#include "settings.hpp"
#include "save.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace mc {
namespace {
constexpr uint32_t Magic = 0x4650434d; // MCPF
constexpr uint32_t Version = 2;
constexpr size_t Size = 40;

void put(std::array<uint8_t, Size> &data, size_t &at, uint32_t value) {
    for (int i = 0; i < 4; i++)
        data[at++] = uint8_t(value >> (i * 8));
}
uint32_t get(const std::array<uint8_t, Size> &data, size_t &at) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++)
        value |= uint32_t(data[at++]) << (i * 8);
    return value;
}
void putFloat(std::array<uint8_t, Size> &data, size_t &at, float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    put(data, at, bits);
}
float getFloat(const std::array<uint8_t, Size> &data, size_t &at) {
    uint32_t bits = get(data, at);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
std::array<uint8_t, Size> encode(const Preferences &p) {
    std::array<uint8_t, Size> data{};
    size_t at = 0;
    put(data, at, Magic);
    put(data, at, Version);
    put(data, at, p.sequence);
    put(data, at, p.citySeed);
    putFloat(data, at, p.cameraX);
    putFloat(data, at, p.cameraY);
    putFloat(data, at, p.zoom);
    data[at++] = p.volume;
    data[at++] = p.mute;
    data[at++] = p.tutorialSeen;
    data[at++] = p.tool;
    data[at++] = p.overlay;
    data[at++] = p.speed;
    data[at++] = p.selectedSlot;
    data[at++] = p.constructionView;
    put(data, at, crc32(data.data(), Size - 4));
    return data;
}
bool decode(const std::array<uint8_t, Size> &data, Preferences &p) {
    uint32_t stored = uint32_t(data[36]) | uint32_t(data[37]) << 8 | uint32_t(data[38]) << 16 |
                      uint32_t(data[39]) << 24;
    if (crc32(data.data(), Size - 4) != stored)
        return false;
    size_t at = 0;
    if (get(data, at) != Magic)
        return false;
    auto version = get(data, at);
    if (version != 1 && version != Version)
        return false;
    Preferences next;
    next.sequence = get(data, at);
    next.citySeed = get(data, at);
    next.cameraX = getFloat(data, at);
    next.cameraY = getFloat(data, at);
    next.zoom = getFloat(data, at);
    next.volume = data[at++];
    next.mute = data[at++];
    next.tutorialSeen = data[at++];
    next.tool = data[at++];
    next.overlay = data[at++];
    next.speed = data[at++];
    next.selectedSlot = data[at++] ? data[at - 1] : 1; // old v1 records reserved this byte
    next.constructionView = version == 1 ? 1 : (data[at++] <= 1 ? data[at - 1] : 1);
    if (next.volume > 100 || next.mute > 1 || next.tutorialSeen > 1 || next.tool > 18 ||
        next.overlay > 7 ||
        (next.speed != 0 && next.speed != 1 && next.speed != 2 && next.speed != 4) ||
        next.selectedSlot > 3 || !std::isfinite(next.cameraX) || !std::isfinite(next.cameraY) ||
        !std::isfinite(next.zoom) || next.cameraX < -6000 || next.cameraX > 7000 ||
        next.cameraY < -8500 || next.cameraY > 550 || next.zoom < .5f || next.zoom > 1.5f)
        return false;
    p = next;
    return true;
}
bool read(const std::string &path, std::array<uint8_t, Size> &data, Preferences &p) {
    FILE *file = fopen(path.c_str(), "rb");
    if (!file)
        return false;
    bool ok = fread(data.data(), 1, data.size(), file) == data.size() && fgetc(file) == EOF;
    fclose(file);
    return ok && decode(data, p);
}
} // namespace

bool loadPreferences(const std::string &root, Preferences &p) {
    std::array<uint8_t, Size> a{}, b{};
    Preferences pa, pb;
    bool av = read(root + "/prefs-a.bin", a, pa), bv = read(root + "/prefs-b.bin", b, pb);
    if (!av && !bv)
        return false;
    p = !av || (bv && pb.sequence > pa.sequence) ? pb : pa;
    return true;
}

bool savePreferences(const std::string &root, Preferences &p) {
    mkdir(root.c_str(), 0775);
    std::array<uint8_t, Size> a{}, b{};
    Preferences pa, pb;
    bool av = read(root + "/prefs-a.bin", a, pa), bv = read(root + "/prefs-b.bin", b, pb);
    uint32_t highest = std::max(p.sequence, std::max(av ? pa.sequence : 0, bv ? pb.sequence : 0));
    if (highest == UINT32_MAX)
        return false;
    uint32_t previous = p.sequence;
    p.sequence = highest + 1;
    std::string target =
        root + (!av || (bv && pa.sequence <= pb.sequence) ? "/prefs-a.bin" : "/prefs-b.bin");
    auto data = encode(p);
    FILE *file = fopen(target.c_str(), "wb");
    bool ok = file && fwrite(data.data(), 1, data.size(), file) == data.size();
    if (file) {
        if (fflush(file))
            ok = false;
        if (fsync(fileno(file)))
            ok = false;
        if (fclose(file))
            ok = false;
    }
    std::array<uint8_t, Size> verify{};
    Preferences checked;
    ok = ok && read(target, verify, checked) && verify == data;
    if (!ok)
        p.sequence = previous;
    return ok;
}
} // namespace mc
