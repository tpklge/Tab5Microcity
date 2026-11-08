#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <atomic>
namespace mc {
class Audio {
    QueueHandle_t queue = nullptr;
    TaskHandle_t task = nullptr;
    static void worker(void *);

  public:
    std::atomic<int> volume{40};
    std::atomic<bool> mute{true};
    bool init();
    void play(const uint8_t *, size_t);
    void stop();
};
} // namespace mc
