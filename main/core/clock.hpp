#pragma once
#include <algorithm>
#include <cstdint>
namespace mc {
// Microseconds. Catch-up is bounded; elapsed suspension is intentionally discarded.
class Clock {
    int64_t accumulated = 0;

  public:
    void reset() {
        accumulated = 0;
    }
    int advance(int64_t elapsed, int speed) {
        if (speed != 1 && speed != 2 && speed != 4)
            return 0;
        accumulated += std::clamp<int64_t>(elapsed, 0, 250000) * speed;
        int ticks = std::min<int64_t>(4, accumulated / 250000);
        accumulated -= int64_t(ticks) * 250000;
        return ticks;
    }
};
} // namespace mc
