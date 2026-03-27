#include "Network.h"
#include <sstream>
#include <algorithm>

std::string Network::get_security_string() const {
    std::vector<std::string> types;
    
    for (const auto& sec : security_types) {
        switch (sec) {
            case SecurityType::OPEN: types.push_back("Open"); break;
            case SecurityType::WEP: types.push_back("WEP"); break;
            case SecurityType::WPA_PSK: types.push_back("WPA"); break;
            case SecurityType::WPA2_PSK: types.push_back("WPA2"); break;
            case SecurityType::WPA3_SAE: types.push_back("WPA3"); break;
            case SecurityType::WPA_WPA2_ENTERPRISE: types.push_back("WPA2E"); break;
            case SecurityType::WPA3_ENTERPRISE: types.push_back("WPA3E"); break;
            default: types.push_back("?"); break;
        }
    }
    
    std::string result;
    for (size_t i = 0; i < types.size(); ++i) {
        if (i > 0) result += "/";
        result += types[i];
    }
    return result;
}

bool SavedNetwork::is_disabled() const {
    return flags.find("DISABLED") != std::string::npos;
}

SecurityType parse_security_type(const std::string& flags) {
    if (flags.find("WPA3") != std::string::npos && flags.find("EAP") != std::string::npos)
        return SecurityType::WPA3_ENTERPRISE;
    if (flags.find("WPA3") != std::string::npos)
        return SecurityType::WPA3_SAE;
    if (flags.find("WPA2") != std::string::npos && flags.find("EAP") != std::string::npos)
        return SecurityType::WPA_WPA2_ENTERPRISE;
    if (flags.find("WPA2") != std::string::npos && flags.find("PSK") != std::string::npos)
        return SecurityType::WPA2_PSK;
    if (flags.find("WPA") != std::string::npos && flags.find("EAP") != std::string::npos)
        return SecurityType::WPA_WPA2_ENTERPRISE;
    if (flags.find("WPA") != std::string::npos)
        return SecurityType::WPA_PSK;
    if (flags.find("WEP") != std::string::npos)
        return SecurityType::WEP;
    if (flags.find("[ESS]") != std::string::npos || flags.empty())
        return SecurityType::OPEN;
    return SecurityType::UNKNOWN;
}
