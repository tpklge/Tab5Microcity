#include "input/controls.hpp"
#include <cassert>
#include <iostream>
using namespace mc;
int main() {
    Keys keys;
    assert(!touchIsDrag(6, 6, false));
    assert(!touchIsDrag(18, 0, false));
    assert(touchIsDrag(19, 0, false));
    assert(!touchIsDrag(4, 4, true));
    assert(touchIsDrag(7, 0, true));
    assert(matrixUsage(2, 2) == 0x1a);  // W
    assert(matrixUsage(3, 11) == 0x52); // Up
    assert(matrixUsage(4, 10) == 0x50); // Left
    assert(matrixUsage(7, 15) == 0);
    keys.set(0, 0x52, true);
    assert(keys.panY() == 1 && keys.pressed(0x52));
    keys.finish();
    assert(!keys.pressed(0x52) && keys.panY() == 1);
    keys.set(0, 0x52, false);
    assert(keys.panY() == 0);
    // A complete fast key tap is not lost between UI frames.
    keys.set(0, 0x1e, true);
    keys.set(0, 0x1e, false);
    assert(keys.pressed(0x1e));
    assert(shortcut(0x1e, keys.pressControl(0x1e), keys.pressAlt(0x1e)).argument == 2);
    keys.finish();
    keys.set(0, 0xe0, true);
    keys.set(0, 0x16, true);
    keys.set(0, 0x16, false);
    keys.set(0, 0xe0, false);
    assert(keys.pressed(0x16) && keys.pressControl(0x16));
    assert(shortcut(0x16, true, false).action == 5);
    keys.finish();
    keys.set(0, 0xe0, true);
    keys.set(0, 0x16, true);
    keys.set(0, 0xe0, false);
    assert(keys.panY() == 0);
    keys.set(0, 0x16, false);
    keys.finish();
    uint8_t report[8] = {0, 0, 0x2c, 0, 0, 0, 0, 0};
    assert(keys.usbReport(report, 8));
    assert(keys.pressed(0x2c));
    keys.finish();
    assert(keys.usbReport(report, 8));
    assert(!keys.pressed(0x2c)); // holding Space never oscillates pause
    report[3] = 0x52;
    assert(keys.usbReport(report, 8));
    assert(!keys.pressed(0x2c) && keys.pressed(0x52));
    assert(keys.panY() == 1);
    keys.finish();
    // Backends combine without USB disconnect releasing a still-held I2C key.
    keys.set(0, 0x52, true);
    keys.clear(1);
    assert(keys.panY() == 1);
    keys.clear(0);
    assert(keys.panY() == 0);
    uint8_t bad[8] = {0, 0, 1, 1, 1, 1, 1, 1};
    assert(!keys.usbReport(bad, 8));
    assert(keys.panY() == 0);
    assert(!keys.usbReport(report, 7));
    for (int key = 0x1e; key <= 0x24; key++) {
        auto cmd = shortcut(key, false, true);
        assert(cmd.action == 18 && cmd.argument == key - 0x1e + 1);
    }
    assert(shortcut(0x27, false, true).argument == 0);
    assert(shortcut(0x4c, false, false).action == 17 &&
           shortcut(0x4c, false, false).argument == 12);
    assert(shortcut(0x16, true, false).action == 5);
    assert(shortcut(0x29, false, false).action == 21);
    assert(shortcut(0x4a, false, false).action == 37);
    assert(shortcut(0x06, false, false).action == 37); // C centers on the Tab5 keyboard
    assert(shortcut(0x05, false, false).action == 39); // B toggles construction view
    assert(shortcut(0x14, false, false).argument == 13); // Q primary school
    assert(shortcut(0x0f, false, false).argument == 14); // L secondary school
    assert(shortcut(0x1c, false, false).argument == 15); // Y university
    assert(shortcut(0x0a, false, false).argument == 16); // G large park
    assert(shortcut(0x11, false, false).argument == 17); // N avenue
    std::cout << "Keyboard: matrix/USB, quick taps, modifiers, held/released keys, disconnect, "
                 "rollover and safe shortcuts: PASS\n";
}
