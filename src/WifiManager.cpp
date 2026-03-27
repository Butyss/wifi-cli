#include "WifiManager.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

WifiManager::WifiManager()
    : initialized_(false)
{
    nm_client_ = std::make_unique<NmCli::NmClient>();
}

WifiManager& WifiManager::get_instance() {
    static WifiManager instance;
    return instance;
}

WifiManager::~WifiManager() {
    cleanup();
}

bool WifiManager::initialize() {
    if (!nm_client_->init()) {
        return false;
    }
    initialized_ = true;
    return true;
}

void WifiManager::cleanup() {
    nm_client_->cleanup();
    initialized_ = false;
}

std::vector<Network> WifiManager::scan() {
    std::vector<Network> networks;

    auto aps = nm_client_->get_access_points(true);
    std::string connected_name = get_current_connection_name();

    for (const auto& ap : aps) {
        Network net = ap_to_network(ap);

        std::string net_ssid = net.ssid;
        while (!net_ssid.empty() && net_ssid.back() == ' ') {
            net_ssid.pop_back();
        }

        if (!connected_name.empty() && net_ssid == connected_name) {
            net.is_connected = true;
            net.is_current = true;
        }

        networks.push_back(net);
    }

    std::sort(networks.begin(), networks.end(),
              [](const Network& a, const Network& b) {
                  return a.signal_strength > b.signal_strength;
              });

    return networks;
}

bool WifiManager::trigger_scan() {
    return nm_client_->scan();
}

std::vector<SavedNetwork> WifiManager::list_networks() {
    std::vector<SavedNetwork> networks;

    FILE* fp = popen("nmcli -t -f NAME,UUID,TYPE connection show 2>/dev/null", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            std::string s = line;
            size_t pos = s.find(':');
            if (pos != std::string::npos) {
                std::string name = s.substr(0, pos);
                std::string rest = s.substr(pos + 1);
                size_t pos2 = rest.find(':');
                std::string uuid = (pos2 != std::string::npos) ? rest.substr(0, pos2) : rest;
                std::string type = (pos2 != std::string::npos) ? rest.substr(pos2 + 1) : "";
                
                if (type.find("wireless") != std::string::npos || type.find("802-11") != std::string::npos) {
                    SavedNetwork net;
                    net.id = uuid;
                    net.ssid = name;
                    
                    while (!net.ssid.empty() && net.ssid.back() == '\n') net.ssid.pop_back();
                    while (!net.ssid.empty() && net.ssid.back() == '\r') net.ssid.pop_back();
                    
                    networks.push_back(net);
                }
            }
        }
        pclose(fp);
    }

    return networks;
}

std::optional<int> WifiManager::connect(const ConnectionConfig& config) {
    if (config.password.empty()) {
        return nm_client_->connect(config.ssid) ? std::make_optional(0) : std::nullopt;
    } else {
        return nm_client_->connect_with_password(config.ssid, config.password)
            ? std::make_optional(0) : std::nullopt;
    }
}

bool WifiManager::disconnect() {
    return nm_client_->disconnect();
}

bool WifiManager::remove_network(const std::string& network_id) {
    std::string cmd = "nmcli connection delete '" + network_id + "' > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

ConnectionStatus WifiManager::get_status() {
    ConnectionStatus status;
    status.state = ConnectionState::UNKNOWN;
    status.signal = 0;

    std::string connected_name = get_current_connection_name();
    
    if (!connected_name.empty()) {
        status.state = ConnectionState::COMPLETED;
        status.ssid = connected_name;
        
        auto aps = nm_client_->get_access_points(false);
        for (const auto& ap : aps) {
            std::string ap_ssid = ap.ssid;
            while (!ap_ssid.empty() && ap_ssid.back() == ' ') {
                ap_ssid.pop_back();
            }
            if (connected_name == ap_ssid) {
                status.signal = ap.strength;
                status.bssid = ap.bssid;
                break;
            }
        }
    } else {
        status.state = ConnectionState::DISCONNECTED;
    }

    return status;
}

std::string WifiManager::get_current_connection_name() {
    FILE* fp = popen("nmcli -t -f NAME,STATE connection show --active 2>/dev/null", "r");
    if (!fp) return "";
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        std::string s = line;
        size_t colon = s.find(':');
        if (colon != std::string::npos) {
            std::string name = s.substr(0, colon);
            std::string state = s.substr(colon + 1);
            
            while (!name.empty() && (name.back() == '\n' || name.back() == '\r')) {
                name.pop_back();
            }
            
            if (state.find("activated") != std::string::npos) {
                pclose(fp);
                return name;
            }
        }
    }
    pclose(fp);
    return "";
}

int WifiManager::get_ping() {
    FILE* fp = popen("ping -c 1 -W 1 8.8.8.8 2>/dev/null | grep -oP 'time=\\K[0-9]+'", "r");
    if (!fp) return -1;
    
    char buf[32];
    if (fgets(buf, sizeof(buf), fp) != nullptr) {
        pclose(fp);
        std::string output(buf);
        output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
        output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());
        try {
            return std::stoi(output);
        } catch (...) {
            return -1;
        }
    }
    pclose(fp);
    return -1;
}

std::string WifiManager::get_password(const std::string& network_id) {
    auto pwd = nm_client_->get_saved_password(network_id);
    if (pwd && !pwd->empty()) {
        return *pwd;
    }
    
    FILE* fp = popen(("nmcli -s -g 802-11-wireless-security.psk connection show '" + network_id + "' 2>/dev/null").c_str(), "r");
    if (!fp) return "";
    
    char buf[256];
    if (fgets(buf, sizeof(buf), fp) != nullptr) {
        pclose(fp);
        std::string result = buf;
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        if (!result.empty() && result != "--") {
            return result;
        }
    }
    pclose(fp);
    return "";
}

Network WifiManager::ap_to_network(const NmCli::AccessPoint& ap) {
    Network net;
    net.ssid = ap.ssid;
    net.bssid = ap.bssid;
    net.signal_strength = ap.strength;
    net.frequency = ap.frequency;
    net.security_types = {flags_to_security(ap.flags)};
    net.id = ap.bssid;
    net.is_connected = false;
    net.is_current = ap.in_use;
    return net;
}

SecurityType WifiManager::flags_to_security(uint32_t flags) {
    const uint32_t NM_802_11_AP_SEC_KEY_MGMT_PSK = 0x1;
    const uint32_t NM_802_11_AP_SEC_KEY_MGMT_SAE = 0x4;
    const uint32_t NM_802_11_AP_SEC_KEY_MGMT_EAP = 0x40;
    const uint32_t NM_802_11_AP_SEC_KEY_MGMT_OWE = 0x100;
    const uint32_t NM_802_11_AP_SEC_PSK = 0x2;
    const uint32_t NM_802_11_AP_SEC_SAE = 0x20;
    const uint32_t NM_802_11_AP_SEC_EAP = 0x80;

    if (flags & NM_802_11_AP_SEC_KEY_MGMT_SAE ||
        flags & NM_802_11_AP_SEC_SAE) {
        return SecurityType::WPA3_SAE;
    }

    if (flags & NM_802_11_AP_SEC_KEY_MGMT_OWE) {
        return SecurityType::WPA3_SAE;
    }

    if (flags & NM_802_11_AP_SEC_KEY_MGMT_EAP ||
        flags & NM_802_11_AP_SEC_EAP) {
        if (flags & NM_802_11_AP_SEC_KEY_MGMT_SAE) {
            return SecurityType::WPA3_ENTERPRISE;
        }
        return SecurityType::WPA_WPA2_ENTERPRISE;
    }

    if (flags & NM_802_11_AP_SEC_KEY_MGMT_PSK ||
        flags & NM_802_11_AP_SEC_PSK) {
        return SecurityType::WPA2_PSK;
    }

    return SecurityType::OPEN;
}
