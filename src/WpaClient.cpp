#include "WpaClient.h"
#include <cstring>
#include <unistd.h>

WpaClient::WpaClient() : ctrl_(nullptr), mon_(nullptr) {}

WpaClient::~WpaClient() {
    disconnect();
}

bool WpaClient::connect(const std::string& iface) {
    ctrl_path_ = "/var/run/wpa_supplicant/" + iface;

    ctrl_ = wpa_ctrl_open(ctrl_path_.c_str());
    if (!ctrl_) {
        ctrl_path_ = "/run/wpa_supplicant/" + iface;
        ctrl_ = wpa_ctrl_open(ctrl_path_.c_str());
    }
    
    if (!ctrl_) {
        return false;
    }

    mon_ = wpa_ctrl_open(ctrl_path_.c_str());
    if (!mon_) {
        wpa_ctrl_close(ctrl_);
        ctrl_ = nullptr;
        return false;
    }

    return true;
}

void WpaClient::disconnect() {
    if (mon_) {
        wpa_ctrl_detach(mon_);
        wpa_ctrl_close(mon_);
        mon_ = nullptr;
    }
    if (ctrl_) {
        wpa_ctrl_close(ctrl_);
        ctrl_ = nullptr;
    }
}

bool WpaClient::send_command(const std::string& cmd, std::string& reply) {
    if (!ctrl_)
        return false;

    char buf[8192];
    size_t len = sizeof(buf) - 1;

    int ret = wpa_ctrl_request(ctrl_, cmd.c_str(), cmd.length(),
                               buf, &len, nullptr);
    
    if (ret == -2) {
        return false;
    }
    if (ret != 0) {
        return false;
    }

    buf[len] = '\0';
    reply = buf;
    return true;
}

bool WpaClient::send_command(const std::string& cmd) {
    std::string reply;
    return send_command(cmd, reply);
}

bool WpaClient::attach() {
    if (!mon_)
        return false;
    return wpa_ctrl_attach(mon_) == 0;
}

bool WpaClient::detach() {
    if (!mon_)
        return false;
    return wpa_ctrl_detach(mon_) == 0;
}

bool WpaClient::recv_event(std::string& event, int timeout_ms) {
    if (!mon_)
        return false;

    char buf[4096];
    size_t len = sizeof(buf) - 1;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(wpa_ctrl_get_fd(mon_), &rfds);

    int sel = select(wpa_ctrl_get_fd(mon_) + 1, &rfds, nullptr, nullptr, &tv);
    if (sel <= 0)
        return false;

    int ret = wpa_ctrl_recv(mon_, buf, &len);
    if (ret != 0)
        return false;

    buf[len] = '\0';
    event = buf;
    return true;
}

bool WpaClient::has_pending_events() {
    if (!mon_)
        return false;
    return wpa_ctrl_pending(mon_) > 0;
}

int WpaClient::get_fd() const {
    if (!mon_)
        return -1;
    return wpa_ctrl_get_fd(mon_);
}
