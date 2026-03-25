#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "WpaClient.h"
#include "Network.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

class WifiManager {
public:
    WifiManager();
    ~WifiManager();

    bool initialize(const std::string& iface = "wlan0");
    void cleanup();

    std::vector<Network> scan();
    bool trigger_scan();

    std::vector<SavedNetwork> list_networks();

    std::optional<int> connect(const ConnectionConfig& config);
    bool disconnect();
    bool remove_network(const std::string& network_id);
    bool enable_network(const std::string& network_id);
    bool disable_network(const std::string& network_id);

    ConnectionStatus get_status();
    std::string get_password(const std::string& network_id);

    void set_event_callback(std::function<void(const std::string&)> callback);
    bool process_events(int timeout_ms = 100);
    void start_monitoring();
    void stop_monitoring();

    bool is_connected_to_wpa() const { return wpa_client_.is_connected(); }

private:
    std::optional<int> add_network();
    bool set_network_string(int network_id, const std::string& key, const std::string& value);
    bool set_network_password(int network_id, const std::string& password, SecurityType type);
    bool enable_network_by_id(int network_id);
    bool wait_for_event(const std::string& expected, int timeout_ms = 15000);
    std::string parse_ssid_from_bss(const std::string& bssid);
    std::vector<SecurityType> parse_security_types(const std::string& flags);

    WpaClient wpa_client_;
    std::function<void(const std::string&)> event_callback_;
    bool monitoring_;
};

#endif
