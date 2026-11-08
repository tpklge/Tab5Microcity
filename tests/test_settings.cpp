#include "storage/settings.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace mc;
namespace fs = std::filesystem;

int main() {
    auto root = fs::temp_directory_path() / "microcity-settings-tests";
    fs::remove_all(root);
    Preferences original;
    original.citySeed = 123456;
    original.cameraX = -250;
    original.cameraY = -900;
    original.zoom = 1.25f;
    original.volume = 70;
    original.mute = 0;
    original.tutorialSeen = 1;
    original.tool = 8;
    original.overlay = 3;
    original.speed = 4;
    original.selectedSlot = 3;
    assert(savePreferences(root.string(), original));
    Preferences loaded;
    assert(loadPreferences(root.string(), loaded));
    assert(loaded.sequence == 1 && loaded.citySeed == 123456 && loaded.cameraX == -250 &&
           loaded.cameraY == -900 && loaded.zoom == 1.25f && loaded.volume == 70 &&
           loaded.mute == 0 && loaded.tutorialSeen == 1 && loaded.tool == 8 &&
           loaded.overlay == 3 && loaded.speed == 4);
    assert(loaded.selectedSlot == 3);
    original.volume = 20;
    assert(savePreferences(root.string(), original) && original.sequence == 2);
    auto newest = root / "prefs-b.bin";
    std::fstream file(newest, std::ios::in | std::ios::out | std::ios::binary);
    file.seekp(20);
    char broken = 99;
    file.write(&broken, 1);
    file.close();
    assert(loadPreferences(root.string(), loaded));
    assert(loaded.sequence == 1 && loaded.volume == 70); // fallback to previous complete slot
    fs::resize_file(root / "prefs-a.bin", 5);
    assert(!loadPreferences(root.string(), loaded));
    fs::remove_all(root);
    std::cout << "SD preferences A/B, validation and corruption fallback: PASS\n";
}
