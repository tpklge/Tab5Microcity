#pragma once
#include "controls.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <atomic>
namespace mc {
class Keyboard {
    struct Input {
        uint8_t kind = 0;
        uint8_t data[8]{};
    };
    QueueHandle_t queue = nullptr, devices = nullptr;
    std::atomic<bool> overflow{false};
    static void run(void *);
    void post(const Input &);

  public:
    std::atomic<bool> tab5Connected{false}, usbConnected{false}, tab5BusReady{false},
        tab5ProbeSeen{false};
    std::atomic<uint8_t> lastUsage{0};
    bool init();
    void poll(Keys &);
};
} // namespace mc
