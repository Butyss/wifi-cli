#include "WifiManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <thread>
#include <chrono>

WifiManager::WifiManager() : monitoring_(false) {}

WifiManager::~WifiManager() {
    cleanup();
}

bool WifiManager::initialize(const std::string& iface) {
    if (!wpa_client_.connect(iface)) {
        std::cerr << "Error: No se pudo conectar a wpa_supplicant." << std::endl;
        std::cerr << "Asegurate de que wpa_supplicant este corriendo en la interfaz " << iface << std::endl;
        return false;
    }

    if (!wpa_client_.attach()) {
        std::cerr << "Error: No se pudo adjuntar al monitor de eventos." << std::endl;
        return false;
    }

    return true;
}

void WifiManager::cleanup() {
    stop_monitoring();
    wpa_client_.disconnect();
}

std::vector<Network> WifiManager::scan() {
    std::vector<Network> networks;

    std::string reply;
    if (!wpa_client_.send_command("SCAN_RESULTS", reply)) {
        std::cerr << "Error al obtener resultados del escaneo" << std::endl;
        return networks;
    }

    std::istringstream stream(reply);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.empty() || line.find("bssid") != std::string::npos)
            continue;

        std::istringstream iss(line);
        Network net;
        
        if (!(iss >> net.bssid >> net.frequency >> net.signal_strength >> net.flags)) {
            continue;
        }

        net.security_types = parse_security_types(net.flags);
        net.ssid = parse_ssid_from_bss(net.bssid);
        net.id = net.bssid;
        net.is_connected = false;
        net.is_current = false;

        if (!net.ssid.empty()) {
            networks.push_back(net);
        }
    }

    ConnectionStatus status = get_status();
    if (status.state == ConnectionState::COMPLETED) {
        for (auto& net : networks) {
            if (net.bssid == status.bssid) {
                net.is_connected = true;
                net.is_current = true;
                break;
            }
        }
    }

    std::sort(networks.begin(), networks.end(), 
              [](const Network& a, const Network& b) {
                  return a.signal_strength > b.signal_strength;
              });

    return networks;
}

bool WifiManager::trigger_scan() {
    std::string reply;
    if (!wpa_client_.send_command("SCAN", reply)) {
        return false;
    }
    return reply.find("OK") != std::string::npos;
}

std::vector<SavedNetwork> WifiManager::list_networks() {
    std::vector<SavedNetwork> networks;

    std::string reply;
    if (!wpa_client_.send_command("LIST_NETWORKS", reply)) {
        return networks;
    }

    std::istringstream stream(reply);
    std::string line;
    bool first = true;

    while (std::getline(stream, line)) {
        if (first) {
            first = false;
            continue;
        }
        if (line.empty())
            continue;

        std::istringstream iss(line);
        SavedNetwork net;

        if (iss >> net.id >> net.ssid >> net.bssid >> net.flags) {
            networks.push_back(net);
        }
    }

    return networks;
}

std::optional<int> WifiManager::connect(const ConnectionConfig& config) {
    auto network_id = add_network();
    if (!network_id.has_value()) {
        return std::nullopt;
    }

    int nid = network_id.value();

    if (!set_network_string(nid, "ssid", "\"" + config.ssid + "\"")) {
        wpa_client_.send_command("REMOVE_NETWORK " + std::to_string(nid));
        return std::nullopt;
    }

    if (!config.is_enterprise) {
        if (!set_network_password(nid, config.password, config.security_type)) {
            wpa_client_.send_command("REMOVE_NETWORK " + std::to_string(nid));
            return std::nullopt;
        }
    } else {
        std::string key_mgmt;
        switch (config.security_type) {
            case SecurityType::WPA3_ENTERPRISE:
                key_mgmt = "WPA-EAP-SHA256";
                break;
            default:
                key_mgmt = "WPA-EAP";
                break;
        }
        set_network_string(nid, "key_mgmt", key_mgmt);
        
        if (!config.identity.empty()) {
            set_network_string(nid, "identity", "\"" + config.identity + "\"");
        }
        if (!config.password.empty()) {
            set_network_string(nid, "password", "\"" + config.password + "\"");
        }
        if (!config.ca_cert.empty()) {
            set_network_string(nid, "ca_cert", "\"" + config.ca_cert + "\"");
        }
        if (!config.client_cert.empty()) {
            set_network_string(nid, "client_cert", "\"" + config.client_cert + "\"");
        }
        if (!config.private_key.empty()) {
            set_network_string(nid, "private_key", "\"" + config.private_key + "\"");
        }
        if (!config.phase2.empty()) {
            set_network_string(nid, "phase2", "\"" + config.phase2 + "\"");
        }
    }

    if (!enable_network_by_id(nid)) {
        wpa_client_.send_command("REMOVE_NETWORK " + std::to_string(nid));
        return std::nullopt;
    }

    if (!wait_for_event("CTRL-EVENT-CONNECTED", 30000)) {
        std::cerr << "Timeout esperando conexion" << std::endl;
        wpa_client_.send_command("REMOVE_NETWORK " + std::to_string(nid));
        return std::nullopt;
    }

    wpa_client_.send_command("SAVE_CONFIG");

    return nid;
}

bool WifiManager::disconnect() {
    return wpa_client_.send_command("DISCONNECT");
}

bool WifiManager::remove_network(const std::string& network_id) {
    std::string reply;
    return wpa_client_.send_command("REMOVE_NETWORK " + network_id, reply);
}

bool WifiManager::enable_network(const std::string& network_id) {
    std::string reply;
    return wpa_client_.send_command("ENABLE_NETWORK " + network_id, reply);
}

bool WifiManager::disable_network(const std::string& network_id) {
    std::string reply;
    return wpa_client_.send_command("DISABLE_NETWORK " + network_id, reply);
}

ConnectionStatus WifiManager::get_status() {
    ConnectionStatus status;
    status.state = ConnectionState::UNKNOWN;
    status.signal = 0;

    std::string reply;
    if (!wpa_client_.send_command("STATUS", reply)) {
        return status;
    }

    status.raw = reply;

    std::istringstream stream(reply);
    std::string line;

    while (std::getline(stream, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if (key == "wpa_state") {
            if (value == "COMPLETED") status.state = ConnectionState::COMPLETED;
            else if (value == "DISCONNECTED") status.state = ConnectionState::DISCONNECTED;
            else if (value == "INACTIVE") status.state = ConnectionState::INACTIVE;
            else if (value == "SCANNING") status.state = ConnectionState::SCANNING;
            else if (value == "AUTHENTICATING") status.state = ConnectionState::AUTHENTICATING;
            else if (value == "ASSOCIATING") status.state = ConnectionState::ASSOCIATING;
            else if (value == "ASSOCIATED") status.state = ConnectionState::ASSOCIATED;
            else if (value == "4WAY_HANDSHAKE") status.state = ConnectionState::HANDSHAKING;
        } else if (key == "ssid") {
            status.ssid = value;
        } else if (key == "bssid") {
            status.bssid = value;
        } else if (key == "ip_address") {
            status.ip_address = value;
        } else if (key == "key_mgmt") {
            status.security = value;
        }
    }

    return status;
}

std::string WifiManager::get_password(const std::string& network_id) {
    std::string reply;
    
    if (wpa_client_.send_command("GET_NETWORK " + network_id + " psk", reply)) {
        if (!reply.empty() && reply.find("FAIL") == std::string::npos) {
            return reply;
        }
    }
    
    return "";
}

void WifiManager::set_event_callback(std::function<void(const std::string&)> callback) {
    event_callback_ = callback;
}

bool WifiManager::process_events(int timeout_ms) {
    std::string event;
    while (wpa_client_.recv_event(event, timeout_ms)) {
        if (event_callback_) {
            event_callback_(event);
        }
        
        if (event.find("CTRL-EVENT-CONNECTED") != std::string::npos ||
            event.find("CTRL-EVENT-DISCONNECTED") != std::string::npos) {
            return true;
        }
    }
    return false;
}

void WifiManager::start_monitoring() {
    monitoring_ = true;
}

void WifiManager::stop_monitoring() {
    monitoring_ = false;
}

std::optional<int> WifiManager::add_network() {
    std::string reply;
    if (!wpa_client_.send_command("ADD_NETWORK", reply)) {
        return std::nullopt;
    }

    try {
        return std::stoi(reply);
    } catch (...) {
        return std::nullopt;
    }
}

bool WifiManager::set_network_string(int network_id, const std::string& key, const std::string& value) {
    std::string cmd = "SET_NETWORK " + std::to_string(network_id) + " " + key + " " + value;
    std::string reply;
    return wpa_client_.send_command(cmd, reply);
}

bool WifiManager::set_network_password(int network_id, const std::string& password, SecurityType type) {
    std::string key_mgmt;
    
    switch (type) {
        case SecurityType::OPEN:
            key_mgmt = "NONE";
            break;
        case SecurityType::WEP:
            key_mgmt = "NONE";
            set_network_string(network_id, "wep_key0", password);
            set_network_string(network_id, "wep_tx_keyidx", "0");
            break;
        case SecurityType::WPA_PSK:
            key_mgmt = "WPA-PSK";
            break;
        case SecurityType::WPA2_PSK:
            key_mgmt = "WPA-PSK";
            break;
        case SecurityType::WPA3_SAE:
            key_mgmt = "SAE";
            break;
        default:
            key_mgmt = "WPA-PSK";
            break;
    }

    if (!key_mgmt.empty()) {
        if (!set_network_string(network_id, "key_mgmt", key_mgmt)) {
            return false;
        }
    }

    if (type != SecurityType::OPEN && type != SecurityType::WEP) {
        if (password.length() >= 8 && password.length() <= 63) {
            return set_network_string(network_id, "psk", "\"" + password + "\"");
        } else if (password.length() == 64) {
            return set_network_string(network_id, "psk", password);
        }
    }

    return true;
}

bool WifiManager::enable_network_by_id(int network_id) {
    std::string reply;
    std::string cmd = "ENABLE_NETWORK " + std::to_string(network_id);
    return wpa_client_.send_command(cmd, reply);
}

bool WifiManager::wait_for_event(const std::string& expected, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    std::string event;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        int remaining = timeout_ms - static_cast<int>(elapsed);
        
        if (remaining <= 0)
            return false;

        if (wpa_client_.recv_event(event, std::min(remaining, 500))) {
            if (event_callback_) {
                event_callback_(event);
            }
            
            if (event.find(expected) != std::string::npos) {
                return true;
            }
            
            if (event.find("CTRL-EVENT-DISCONNECTED") != std::string::npos) {
                return false;
            }
        }
    }
}

std::string WifiManager::parse_ssid_from_bss(const std::string& bssid) {
    std::string reply;
    if (!wpa_client_.send_command("BSS " + bssid, reply)) {
        return "";
    }

    std::istringstream stream(reply);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.substr(0, 5) == "ssid=") {
            return line.substr(5);
        }
    }
    return "";
}

std::vector<SecurityType> WifiManager::parse_security_types(const std::string& flags) {
    std::vector<SecurityType> types;
    
    if (flags.find("WPA3") != std::string::npos) {
        if (flags.find("EAP") != std::string::npos) {
            types.push_back(SecurityType::WPA3_ENTERPRISE);
        } else {
            types.push_back(SecurityType::WPA3_SAE);
        }
    }
    
    if (flags.find("WPA2") != std::string::npos) {
        if (flags.find("EAP") != std::string::npos) {
            if (types.empty()) types.push_back(SecurityType::WPA_WPA2_ENTERPRISE);
        } else {
            types.push_back(SecurityType::WPA2_PSK);
        }
    }
    
    if (flags.find("WPA-") != std::string::npos || flags.find("WPA ") != std::string::npos) {
        if (flags.find("EAP") != std::string::npos) {
            if (types.empty()) types.push_back(SecurityType::WPA_WPA2_ENTERPRISE);
        } else {
            types.push_back(SecurityType::WPA_PSK);
        }
    }
    
    if (flags.find("WEP") != std::string::npos) {
        types.push_back(SecurityType::WEP);
    }
    
    if (types.empty()) {
        types.push_back(SecurityType::OPEN);
    }
    
    return types;
}
