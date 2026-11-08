#include "keyboard.hpp"
#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "m5_tab5_keyboard.h"
#include "usb/hid_host.h"
#include <cstring>
namespace mc {
namespace {
struct DeviceEvent {
    hid_host_device_handle_t handle;
    hid_host_driver_event_t event;
};
} // namespace
bool Keyboard::init() {
    queue = xQueueCreate(64, sizeof(Input));
    devices = xQueueCreate(8, sizeof(DeviceEvent));
    if (!queue || !devices) {
        if (queue)
            vQueueDelete(queue);
        if (devices)
            vQueueDelete(devices);
        queue = devices = nullptr;
        return false;
    }
    if (xTaskCreate(run, "mc-keyboard", 6144, this, 4, nullptr) != pdPASS) {
        vQueueDelete(queue);
        vQueueDelete(devices);
        queue = devices = nullptr;
        return false;
    }
    return true;
}
void Keyboard::post(const Input &input) {
    if (xQueueSend(queue, &input, 0) != pdTRUE)
        overflow = true;
}
void Keyboard::poll(Keys &keys) {
    if (!queue)
        return;
    if (overflow.exchange(false)) {
        xQueueReset(queue);
        keys.clear(0);
        keys.clear(1);
        return;
    }
    Input event;
    while (xQueueReceive(queue, &event, 0) == pdTRUE) {
        switch (event.kind) {
        case 0:
            keys.set(0, event.data[0], event.data[1]);
            if (event.data[1])
                lastUsage = event.data[0];
            break;
        case 1:
            keys.usbReport(event.data, 8);
            for (int i = 2; i < 8; i++)
                if (event.data[i])
                    lastUsage = event.data[i];
            break;
        case 2:
            keys.clear(0);
            break;
        case 3:
            keys.clear(1);
            break;
        }
    }
}
void Keyboard::run(void *arg) {
    auto self = (Keyboard *)arg;
    i2c_master_bus_handle_t bus = nullptr;
    i2c_master_bus_config_t config{};
    // M5Stack's A164 reference firmware uses I2C1 for the keyboard connector.
    // The board BSP is configured on I2C0 so both buses can coexist.
    config.i2c_port = I2C_NUM_1;
    config.sda_io_num = GPIO_NUM_0;
    config.scl_io_num = GPIO_NUM_1;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;
    esp_err_t busResult = i2c_new_master_bus(&config, &bus);
    self->tab5BusReady = busResult == ESP_OK;
    if (busResult != ESP_OK)
        ESP_LOGE("keyboard", "Tab5 keyboard I2C init: %s", esp_err_to_name(busResult));
    m5::M5Tab5Keyboard tab5;
    m5::M5Tab5Keyboard::setLogLevel(M5_TAB5_KB_LOG_LEVEL_NONE);
    if (bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, false) == ESP_OK) {
        hid_host_driver_config_t cfg{};
        cfg.create_background_task = true;
        cfg.task_priority = 5;
        cfg.stack_size = 4096;
        cfg.core_id = 0;
        cfg.callback_arg = self;
        cfg.callback = [](hid_host_device_handle_t handle, hid_host_driver_event_t event,
                          void *context) {
            auto k = (Keyboard *)context;
            DeviceEvent d{handle, event};
            if (xQueueSend(k->devices, &d, 0) != pdTRUE)
                k->overflow = true;
        };
        auto result = hid_host_install(&cfg);
        if (result != ESP_OK)
            ESP_LOGW("keyboard", "HID unavailable: %s", esp_err_to_name(result));
    }
    int64_t nextProbe = 0;
    for (;;) {
        DeviceEvent d;
        if (xQueueReceive(self->devices, &d, pdMS_TO_TICKS(20)) == pdTRUE &&
            d.event == HID_HOST_DRIVER_EVENT_CONNECTED) {
            hid_host_dev_params_t params{};
            if (hid_host_device_get_params(d.handle, &params) == ESP_OK &&
                params.sub_class == HID_SUBCLASS_BOOT_INTERFACE &&
                params.proto == HID_PROTOCOL_KEYBOARD) {
                hid_host_device_config_t dc{};
                dc.callback_arg = self;
                dc.callback = [](hid_host_device_handle_t handle, hid_host_interface_event_t event,
                                 void *context) {
                    auto k = (Keyboard *)context;
                    if (event == HID_HOST_INTERFACE_EVENT_INPUT_REPORT) {
                        Input input;
                        input.kind = 1;
                        size_t n = 0;
                        if (hid_host_device_get_raw_input_report_data(handle, input.data, 8, &n) ==
                                ESP_OK &&
                            n == 8)
                            k->post(input);
                    } else if (event == HID_HOST_INTERFACE_EVENT_DISCONNECTED) {
                        k->usbConnected = false;
                        Input input;
                        input.kind = 3;
                        k->post(input);
                        hid_host_device_close(handle);
                    } else if (event == HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR) {
                        Input input;
                        input.kind = 3;
                        k->post(input);
                    }
                };
                if (hid_host_device_open(d.handle, &dc) == ESP_OK) {
                    auto result =
                        hid_class_request_set_protocol(d.handle, HID_REPORT_PROTOCOL_BOOT);
                    if (result == ESP_OK)
                        result = hid_class_request_set_idle(d.handle, 0, 0);
                    if (result == ESP_OK)
                        result = hid_host_device_start(d.handle);
                    if (result == ESP_OK) {
                        self->usbConnected = true;
                        ESP_LOGI("keyboard", "USB keyboard connected");
                    } else {
                        hid_host_device_close(d.handle);
                        ESP_LOGW("keyboard", "USB keyboard start: %s", esp_err_to_name(result));
                    }
                }
            }
        }
        auto now = esp_timer_get_time();
        if (busResult == ESP_OK && !tab5.isInitialized() && now >= nextProbe) {
            nextProbe = now + 2000000;
            if (i2c_master_probe(bus, 0x6d, 5) == ESP_OK &&
                tab5.begin(bus, 0x6d, 100000, M5_TAB5_KB_INT_MODE_DISABLED) == M5_TAB5_KB_OK) {
                if (tab5.enableNormalMode() == M5_TAB5_KB_OK) {
                    tab5.clearEventQueue();
                    self->tab5Connected = true;
                    self->tab5ProbeSeen = true;
                    ESP_LOGI("keyboard", "Tab5 I2C keyboard connected");
                } else
                    tab5.end();
            }
        }
        if (tab5.isInitialized()) {
            uint8_t count = 0;
            bool valid = tab5.getEventCount(&count) == M5_TAB5_KB_OK;
            for (int i = 0; valid && i < count && i < 32; i++) {
                uint8_t raw = 0xff;
                valid = tab5.getKeyEvent(&raw) == M5_TAB5_KB_OK;
                if (valid && raw != 0xff) {
                    Input input;
                    input.kind = 0;
                    input.data[0] = matrixUsage((raw >> 4) & 7, raw & 15);
                    input.data[1] = (raw & 0x80) != 0;
                    self->post(input);
                }
            }
            if (!valid) {
                tab5.end();
                self->tab5Connected = false;
                Input input;
                input.kind = 2;
                self->post(input);
                nextProbe = now + 2000000;
            }
        }
    }
}
} // namespace mc
