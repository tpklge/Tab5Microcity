#include "receiver.hpp"
#include "bsp/esp-bsp.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "storage/assets.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
namespace mc {
bool Receiver::start(const std::string &r) {
    if (server)
        return true;
    auto fail = [&]() {
        stop();
        return false;
    };
    root = r;
    result = 0;
    progress = 0;
    cancelled = false;
    uint32_t a = esp_random(), b = esp_random(), c = esp_random(), d = esp_random();
    char buf[40];
    snprintf(buf, sizeof(buf), "%08lx%08lx%08lx%08lx", (unsigned long)a, (unsigned long)b,
             (unsigned long)c, (unsigned long)d);
    token = buf;
    snprintf(buf, sizeof(buf), "MC%08lx%08lx", (unsigned long)esp_random(),
             (unsigned long)esp_random());
    password = buf;
    static bool initialized = false;
    static esp_netif_t *accessPoint = nullptr;
    if (!initialized) {
        if (esp_netif_init() != ESP_OK)
            return fail();
        auto e = esp_event_loop_create_default();
        if (e != ESP_OK && e != ESP_ERR_INVALID_STATE)
            return fail();
        if (!accessPoint)
            accessPoint = esp_netif_create_default_wifi_ap();
        if (!accessPoint)
            return fail();
        if (bsp_feature_enable(BSP_FEATURE_WIFI, true) != ESP_OK)
            return fail();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        if (esp_wifi_init(&cfg) != ESP_OK)
            return fail();
        initialized = true;
    }
    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK)
        return fail();
    wifi_config_t cfg{};
    strcpy((char *)cfg.ap.ssid, "MicroCity-Tab5");
    strcpy((char *)cfg.ap.password, password.c_str());
    cfg.ap.ssid_len = strlen((char *)cfg.ap.ssid);
    cfg.ap.channel = 6;
    cfg.ap.max_connection = 1;
    cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_AP, &cfg) != ESP_OK || esp_wifi_start() != ESP_OK)
        return fail();
    wifi = true;
    httpd_config_t hc = HTTPD_DEFAULT_CONFIG();
    hc.stack_size = 24576;
    hc.max_uri_handlers = 2;
    hc.recv_wait_timeout = 5;
    hc.send_wait_timeout = 5;
    hc.max_open_sockets = 2;
    if (httpd_start(&server, &hc) != ESP_OK) {
        stop();
        return fail();
    }
    httpd_uri_t route{};
    route.uri = "/assets";
    route.method = HTTP_POST;
    route.handler = upload;
    route.user_ctx = this;
    if (httpd_register_uri_handler(server, &route) != ESP_OK) {
        stop();
        return fail();
    }
    return true;
}
const char *Receiver::statusText() const {
    switch (result.load()) {
    case 0:
        return "Aguardando envio";
    case 1:
        return "Recebendo e verificando";
    case 2:
        return "Instalação concluída";
    case -1:
        return "Token inválido";
    case -2:
        return "Pacote excede limite";
    case -3:
        return "Sequência esgotada";
    case -4:
        return "Falha no cartão";
    case -5:
        return "Espaço insuficiente";
    case -6:
        return "Transferência interrompida";
    case -7:
        return "Pacote inválido/incompatível";
    default:
        return "Falha ao ativar pacote";
    }
}
void Receiver::stop() {
    cancelled = true;
    if (server) {
        httpd_stop(server);
        server = nullptr;
    }
    if (wifi) {
        esp_wifi_stop();
        wifi = false;
    }
    token.clear();
    password.clear();
}
esp_err_t Receiver::upload(httpd_req_t *req) {
    auto self = (Receiver *)req->user_ctx;
    auto fail = [&](const char *status, const char *text, int result) {
        self->result = result;
        httpd_resp_set_status(req, status);
        return httpd_resp_send(req, text, HTTPD_RESP_USE_STRLEN);
    };
    char auth[80];
    std::string expected = "Bearer " + self->token;
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth, sizeof(auth)) != ESP_OK ||
        expected != auth)
        return fail("401 Unauthorized", "Token inválido", -1);
    if (req->content_len < 60 || req->content_len > 32 * 1024 * 1024)
        return fail("413 Payload Too Large", "Limite: 32 MiB", -2);
    self->progress = 0;
    self->result = 1;
    std::string active;
    selectAssets(self->root, active);
    int slot = active == self->root + "/slot-a.mcap" ? 1 : 0;
    auto target = self->root + (slot ? "/slot-b.mcap" : "/slot-a.mcap");
    auto ready = self->root + (slot ? "/slot-b.ready" : "/slot-a.ready");
    uint32_t seq = std::max(assetSequence(self->root, 0), assetSequence(self->root, 1));
    if (seq == UINT32_MAX)
        return fail("409 Conflict", "Sequência esgotada", -3);
    // The active bundle is never touched. An incomplete inactive slot has no ready record.
    if (unlink(ready.c_str()) && errno != ENOENT)
        return fail("500 Internal Server Error", "Falha no cartão", -4);
    unlink(target.c_str());
    uint64_t total = 0, free = 0;
    if (esp_vfs_fat_info("/sdcard", &total, &free) != ESP_OK ||
        free < uint64_t(req->content_len) + 65536)
        return fail("507 Insufficient Storage", "Espaço insuficiente", -5);
    FILE *f = fopen(target.c_str(), "wb");
    if (!f)
        return fail("500 Internal Server Error", "Falha ao abrir SD", -4);
    char buf[4096];
    int left = req->content_len;
    bool ok = true;
    while (left && !self->cancelled) {
        int n = httpd_req_recv(req, buf, std::min(left, int(sizeof(buf))));
        if (n <= 0 || fwrite(buf, 1, n, f) != size_t(n)) {
            ok = false;
            break;
        }
        left -= n;
        self->progress = (req->content_len - left) * 90 / req->content_len;
    }
    if (fflush(f) || fsync(fileno(f)))
        ok = false;
    if (fclose(f))
        ok = false;
    if (!ok || left || self->cancelled)
        return fail("408 Request Timeout", "Recepção interrompida; versão anterior preservada", -6);
    std::string err;
    uint32_t version;
    if (!Assets::validate(target, version, err))
        return fail("422 Unprocessable Content", err.c_str(), -7);
    self->progress = 95;
    if (!publishAssets(self->root, slot, seq + 1))
        return fail("500 Internal Server Error", "Falha ao publicar versão", -8);
    // Reopen and hash the completed file after publishing; boot also validates both slots.
    uint32_t verified;
    if (!Assets::validate(target, verified, err)) {
        unlink(ready.c_str());
        return fail("500 Internal Server Error", "Verificação final falhou", -8);
    }
    self->installed = version;
    self->progress = 100;
    self->result = 2;
    char response[100];
    snprintf(response, sizeof(response), "{\"status\":\"installed\",\"version\":%lu}",
             (unsigned long)version);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, response);
}
} // namespace mc
