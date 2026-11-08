#include "audio.hpp"
#include "bsp/esp-bsp.h"
#include <cstdlib>
#include <cstring>
namespace mc {
struct Sound {
    uint8_t pcm[4800];
};
bool Audio::init() {
    if (queue)
        return true;
    queue = xQueueCreate(2, sizeof(Sound));
    if (!queue)
        return false;
    if (xTaskCreate(worker, "mc-audio", 6144, this, 2, &task) != pdPASS) {
        vQueueDelete(queue);
        queue = nullptr;
        return false;
    }
    return true;
}
void Audio::play(const uint8_t *p, size_t n) {
    if (!p || n != 4800 || mute || !queue)
        return;
    Sound s;
    memcpy(s.pcm, p, n);
    xQueueSend(queue, &s, 0);
}
void Audio::stop() {
    if (queue)
        xQueueReset(queue);
}
void Audio::worker(void *arg) {
    auto self = (Audio *)arg;
    auto s = (Sound *)malloc(sizeof(Sound));
    esp_codec_dev_handle_t codec = nullptr;
    if (!s) {
        self->mute = true;
        vTaskDelete(nullptr);
        return;
    }
    for (;;) {
        if (xQueueReceive(self->queue, s, portMAX_DELAY) != pdTRUE)
            continue;
        if (self->mute)
            continue;
        if (!codec) {
            codec = bsp_audio_codec_speaker_init();
            if (codec) {
                esp_codec_dev_sample_info_t fmt{};
                fmt.sample_rate = 16000;
                fmt.channel = 1;
                fmt.bits_per_sample = 16;
                if (esp_codec_dev_open(codec, &fmt) != ESP_OK) {
                    esp_codec_dev_delete(codec);
                    codec = nullptr;
                }
            }
        }
        if (codec) {
            esp_codec_dev_set_out_vol(codec, self->volume.load());
            esp_codec_dev_write(codec, s->pcm, sizeof(s->pcm));
        }
    }
}
} // namespace mc
