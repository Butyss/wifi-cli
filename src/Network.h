#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <optional>

enum class SecurityType {
    OPEN,
    WEP,
    WPA_PSK,
    WPA2_PSK,
    WPA3_SAE,
    WPA_WPA2_ENTERPRISE,
    WPA3_ENTERPRISE,
    UNKNOWN
};

struct Network {
    std::string id;
    std::string ssid;
    std::string bssid;
    int signal_strength;
    int frequency;
    std::vector<SecurityType> security_types;
    bool is_connected;
    bool is_current;
    std::string flags;

    std::string get_security_string() const;
};

struct SavedNetwork {
    std::string id;
    std::string ssid;
    std::string bssid;
    std::string flags;

    bool is_disabled() const;
};

struct ConnectionConfig {
    std::string ssid;
    std::string password;
    SecurityType security_type;
    bool is_enterprise;
    
    std::string identity;
    std::string ca_cert;
    std::string client_cert;
    std::string private_key;
    std::string private_key_passwd;
    std::string phase1;
    std::string phase2;
};

enum class ConnectionState {
    DISCONNECTED,
    INACTIVE,
    SCANNING,
    AUTHENTICATING,
    ASSOCIATING,
    ASSOCIATED,
    HANDSHAKING,
    COMPLETED,
    UNKNOWN
};

struct ConnectionStatus {
    ConnectionState state;
    std::string ssid;
    std::string bssid;
    std::string ip_address;
    std::string security;
    int signal;
    std::string raw;
};

SecurityType parse_security_type(const std::string& flags);

#endif
