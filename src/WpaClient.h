#ifndef WPA_CLIENT_H
#define WPA_CLIENT_H

#include <string>
#include <vector>
#include <functional>
#include <optional>

extern "C" {
#include "wpa_ctrl.h"
}

class WpaClient {
public:
    WpaClient();
    ~WpaClient();

    bool connect(const std::string& iface = "wlan0");
    void disconnect();

    bool send_command(const std::string& cmd, std::string& reply);
    bool send_command(const std::string& cmd);

    bool attach();
    bool detach();

    bool recv_event(std::string& event, int timeout_ms = 1000);
    bool has_pending_events();

    int get_fd() const;

    bool is_connected() const { return ctrl_ != nullptr; }

private:
    struct wpa_ctrl* ctrl_;
    struct wpa_ctrl* mon_;
    std::string ctrl_path_;
};

#endif
