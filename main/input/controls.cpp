#include "controls.hpp"
namespace mc {
bool touchIsDrag(int dx, int dy, bool camera) {
    int slop = camera ? 6 : 18;
    return int64_t(dx) * dx + int64_t(dy) * dy > slop * slop;
}

uint8_t matrixUsage(uint8_t row, uint8_t col) {
    // Matrix positions documented by M5Stack Tab5 Keyboard UserDemo (MIT).
    static constexpr uint8_t map[5][14] = {
        {0x29, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x4c},
        {0x35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2f, 0x30, 0x31},
        {0x2b, 0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c, 0x12, 0x13, 0x33, 0x34, 0x2a},
        {0, 0xe1, 0x04, 0x16, 0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x52, 0x2e, 0x28},
        {0xe0, 0xe2, 0x1d, 0x1b, 0x06, 0x19, 0x05, 0x11, 0x10, 0x37, 0x50, 0x51, 0x4f, 0x2c}};
    return row < 5 && col < 14 ? map[row][col] : 0;
}
bool Keys::usbReport(const uint8_t *p, size_t n) {
    if (!p || n != 8)
        return false;
    for (int i = 2; i < 8; i++)
        if (p[i] > 0 && p[i] <= 3) {
            clear(1);
            return false;
        }
    std::array<bool, 256> old{}, next{};
    for (int i = 0; i < 256; i++)
        old[i] = down(i);
    for (int i = 0; i < 8; i++)
        next[0xe0 + i] = (p[0] & (1 << i)) != 0;
    for (int i = 2; i < 8; i++)
        if (p[i])
            next[p[i]] = true;
    held[1] = next;
    for (int i = 1; i < 256; i++)
        if (!old[i] && down(i))
            latch(i);
    return true;
}
Shortcut shortcut(uint8_t key, bool ctrl, bool alt) {
    if (ctrl) {
        if (key == 0x16)
            return {5, 0};
        return {};
    }
    if (alt) {
        if (key >= 0x1e && key <= 0x24)
            return {18, key - 0x1e + 1};
        if (key == 0x27)
            return {18, 0};
        return {};
    }
    // 1..9 selects tools, 0 park; letters reserved for utilities/services.
    if (key >= 0x1e && key <= 0x26) {
        static constexpr int tools[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
        return {17, tools[key - 0x1e]};
    }
    if (key == 0x27)
        return {17, 11};
    switch (key) {
    case 0x15:
        return {17, 2}; // R road
    case 0x08:
        return {17, 6}; // E power
    case 0x18:
        return {17, 7}; // U water
    case 0x09:
        return {17, 8}; // F fire
    case 0x13:
        return {17, 9}; // P police
    case 0x0b:
        return {17, 10}; // H health
    case 0x0e:
        return {17, 11}; // K park
    case 0x14:
        return {17, 13}; // Q primary school
    case 0x0f:
        return {17, 14}; // L secondary school
    case 0x1c:
        return {17, 15}; // Y university
    case 0x0a:
        return {17, 16}; // G large park
    case 0x11:
        return {17, 17}; // N avenue
    case 0x17:
        return {17, 18}; // T port
    case 0x05:
        return {39, 0}; // B construction view
    case 0x1b:
    case 0x4c:
        return {17, 12}; // X / Delete selects demolition; never demolishes immediately
    case 0x0c:
        return {17, 1}; // I inspect
    case 0x19:
        return {17, 0}; // V pan
    case 0x12:
        return {19, 0}; // O next overlay
    case 0x2c:
        return {20, 0}; // Space pause toggle
    case 0x2e:
    case 0x57:
        return {4, 1};
    case 0x2d:
    case 0x56:
        return {4, 255};
    case 0x29:
        return {21, 0}; // Escape cancels tool/palette
    case 0x06:
    case 0x4a:
        return {37, 0}; // Home centers the city
    case 0x2b:
        return {22, 0}; // Tab tools panel
    case 0x3a:
        return {23, 0}; // F1 help
    default:
        if (key >= 0x3b && key <= 0x42)
            return {18, key - 0x3b};
        return {};
    }
}
} // namespace mc
