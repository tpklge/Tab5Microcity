#pragma once
#include "esp_http_server.h"
#include <atomic>
#include <string>
namespace mc {
class Receiver {
    httpd_handle_t server = nullptr;
    bool wifi = false;
    std::string root, token, password;
    static esp_err_t upload(httpd_req_t *);

  public:
    std::atomic<int> progress{0}, result{0};
    std::atomic<bool> cancelled{false};
    std::atomic<uint32_t> installed{0};
    const char *statusText() const;
    bool start(const std::string &);
    void stop();
    const std::string &accessToken() const {
        return token;
    }
    const std::string &apPassword() const {
        return password;
    }
};
} // namespace mc
