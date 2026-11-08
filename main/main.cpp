#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ui/application.hpp"
extern "C" void app_main() {
    esp_err_t n = nvs_flash_init();
    if (n != ESP_OK) {
        ESP_LOGE("microcity", "NVS: %s", esp_err_to_name(n));
    }
    bsp_display_cfg_t config{};
    config.lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    config.lvgl_port_cfg.task_stack = 24576;
    config.lvgl_port_cfg.task_max_sleep_ms = 10;
    config.buffer_size = BSP_LCD_H_RES * 50;
    config.double_buffer = true;
    config.flags.buff_dma = true;
    config.flags.sw_rotate = true;
    auto display = bsp_display_start_with_config(&config);
    if (!display) {
        ESP_LOGE("microcity", "Falha ao iniciar display");
        return;
    }
    bsp_display_backlight_on();
    bsp_display_lock(0);
    bsp_display_rotate(display, LV_DISPLAY_ROTATION_90);
    bsp_display_unlock();
    ESP_LOGI("microcity", "baseline display: internal=%u PSRAM=%u",
             unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
    mc::startApplication(bsp_sdcard_mount() == ESP_OK);
}
