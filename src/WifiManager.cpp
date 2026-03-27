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
    return scan_nmcli();
}

std::vector<Network> WifiManager::scan_nmcli() {
    std::vector<Network> networks;
    
    std::string connected_name = get_current_connection_name();
    
    FILE* fp = popen("TERM=dumb nmcli -t device wifi list 2>/dev/null", "r");
    if (!fp) return networks;
    
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        std::string s = line;
        if (s.back() == '\n') s.pop_back();
        if (s.empty()) continue;
        
        if (s[0] == '*' || s[0] == ' ') s = s.substr(1);
        
        size_t mode_pos = s.find(":Infra:");
        std::string mode = "Infra";
        if (mode_pos == std::string::npos) {
            mode_pos = s.find(":Ad-Hoc:");
            mode = "Ad-Hoc";
        }
        if (mode_pos == std::string::npos) {
            mode_pos = s.find(":Mesh:");
            mode = "Mesh";
        }
        
        if (mode_pos == std::string::npos) continue;
        
        std::string before_mode = s.substr(0, mode_pos);
        
        size_t last_colon_before_mode = before_mode.rfind(':');
        if (last_colon_before_mode == std::string::npos) continue;
        
        std::string bssid = before_mode.substr(0, last_colon_before_mode);
        std::string ssid = before_mode.substr(last_colon_before_mode + 1);
        
        std::string after_mode = s.substr(mode_pos + mode.length() + 2);
        
        std::vector<std::string> after_fields;
        size_t pos = 0;
        while ((pos = after_mode.find(':')) != std::string::npos && after_fields.size() < 6) {
            after_fields.push_back(after_mode.substr(0, pos));
            after_mode = after_mode.substr(pos + 1);
        }
        after_fields.push_back(after_mode);
        
        Network net;
        net.bssid = bssid;
        net.ssid = ssid;
        
        for (size_t i = 0; i < after_fields.size(); i++) {
            std::string& f = after_fields[i];
            
            if (f.find("Mbit/s") != std::string::npos) continue;
            
            if (std::isdigit(f[0])) {
                int val = std::atoi(f.c_str());
                if (val > 0 && val <= 100) {
                    net.signal_strength = -100 + (val * 50 / 100);
                } else if (val > 1000) {
                    net.frequency = val;
                }
            }
            
            if (f.find("WPA3") != std::string::npos) {
                net.security_types = {SecurityType::WPA3_SAE, SecurityType::WPA2_PSK};
            } else if (f.find("WPA2") != std::string::npos) {
                net.security_types = {SecurityType::WPA2_PSK};
            } else if (f.find("WPA") != std::string::npos) {
                net.security_types = {SecurityType::WPA_PSK};
            } else if (f.find("WEP") != std::string::npos) {
                net.security_types = {SecurityType::WEP};
            } else if (f == "Open" || f.empty()) {
                if (net.security_types.empty()) {
                    net.security_types = {SecurityType::OPEN};
                }
            }
        }
        
        if (net.ssid.empty()) continue;
        
        while (!net.ssid.empty() && (net.ssid.back() == ' ' || net.ssid.back() == '\\')) {
            net.ssid.pop_back();
        }
        
        while (!net.ssid.empty() && net.ssid[0] == ' ') {
            net.ssid = net.ssid.substr(1);
        }
        
        if (net.signal_strength == 0) net.signal_strength = -100;
        if (net.frequency == 0) net.frequency = 2400;
        if (net.security_types.empty()) net.security_types = {SecurityType::OPEN};
        
        std::string normalized_ssid = net.ssid;
        std::string normalized_conn = connected_name;
        while (!normalized_ssid.empty() && normalized_ssid.back() == ' ') normalized_ssid.pop_back();
        while (!normalized_conn.empty() && normalized_conn.back() == ' ') normalized_conn.pop_back();
        if (!normalized_conn.empty() && normalized_ssid == normalized_conn) {
            net.is_connected = true;
            net.is_current = true;
        }
        
        networks.push_back(net);
    }
    
    pclose(fp);
    
    std::sort(networks.begin(), networks.end(),
              [](const Network& a, const Network& b) {
                  return a.signal_strength > b.signal_strength;
              });
    
    return networks;
}

bool WifiManager::trigger_scan() {
    int result = system("nmcli device wifi rescan 2>/dev/null");
    return result == 0;
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
    last_error_.clear();
    
    if (config.password.empty()) {
        if (nm_client_->connect(config.ssid)) {
            return std::make_optional(0);
        } else {
            last_error_ = "Error al conectar";
            return std::nullopt;
        }
    } else {
        bool success = nm_client_->connect_with_password(config.ssid, config.password);
        if (!success) {
            last_error_ = "Clave incorrecta";
        }
        return success ? std::make_optional(0) : std::nullopt;
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
