#pragma once
#include <cstdint>
#include <string>

namespace mc {
struct Preferences {
    uint32_t sequence = 0;
    uint32_t citySeed = 0;
    float cameraX = 640, cameraY = -1785, zoom = 1;
    uint8_t volume = 40, mute = 1, tutorialSeen = 0;
    uint8_t tool = 0, overlay = 0, speed = 1, selectedSlot = 1;
    uint8_t constructionView = 1;
};

bool savePreferences(const std::string &root, Preferences &preferences);
bool loadPreferences(const std::string &root, Preferences &preferences);
} // namespace mc
