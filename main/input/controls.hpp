#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace mc {
// USB usage IDs are shared by the USB and Tab5 matrix adapters.
uint8_t matrixUsage(uint8_t row, uint8_t column);
bool touchIsDrag(int dx, int dy, bool camera);
struct Shortcut {
    int action = 0, argument = 0;
};
Shortcut shortcut(uint8_t usage, bool control, bool alt);
class Keys {
    std::array<std::array<bool, 256>, 2> held{};
    std::array<uint8_t, 256> edges{}, heldModifiers{};
    void latch(uint8_t usage) {
        edges[usage] = 1 | (control() ? 2 : 0) | (alt() ? 4 : 0);
        heldModifiers[usage] = edges[usage] & 6;
    }

  public:
    void set(int source, uint8_t usage, bool value) {
        if (source < 0 || source >= 2 || !usage)
            return;
        bool was = down(usage);
        held[source][usage] = value;
        if (!was && down(usage))
            latch(usage);
    }
    void clear(int source) {
        if (source >= 0 && source < 2)
            held[source].fill(false);
    }
    bool usbReport(const uint8_t *data, size_t length);
    bool down(uint8_t usage) const {
        return held[0][usage] || held[1][usage];
    }
    bool pressed(uint8_t usage) const {
        return edges[usage] & 1;
    }
    bool pressControl(uint8_t usage) const {
        return edges[usage] & 2;
    }
    bool pressAlt(uint8_t usage) const {
        return edges[usage] & 4;
    }
    void finish() {
        edges.fill(0);
    }
    bool movementDown(uint8_t usage) const {
        return down(usage) && !heldModifiers[usage];
    }
    int panX() const {
        return (movementDown(0x50) || movementDown(0x04) ? 1 : 0) -
               (movementDown(0x4f) || movementDown(0x07) ? 1 : 0);
    }
    int panY() const {
        return (movementDown(0x52) || movementDown(0x1a) ? 1 : 0) -
               (movementDown(0x51) || movementDown(0x16) ? 1 : 0);
    }
    bool control() const {
        return down(0xe0) || down(0xe4);
    }
    bool alt() const {
        return down(0xe2) || down(0xe6);
    }
    bool shift() const {
        return down(0xe1) || down(0xe5);
    }
};
} // namespace mc
