#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "NmClient.h"
#include "Network.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

class WifiManager {
public:
    static WifiManager& get_instance();
    
    ~WifiManager();

    bool initialize();
    void cleanup();

    std::vector<Network> scan();
    bool trigger_scan();

    std::vector<SavedNetwork> list_networks();

    std::optional<int> connect(const ConnectionConfig& config);
    bool disconnect();
    bool remove_network(const std::string& network_id);

    ConnectionStatus get_status();
    std::string get_password(const std::string& network_id);

    bool is_connected_to_nm() const { return nm_client_ && nm_client_->init(); }

private:
    WifiManager();
    
    Network ap_to_network(const NmCli::AccessPoint& ap);
    SecurityType flags_to_security(uint32_t flags);

    bool initialized_;
    std::unique_ptr<NmCli::NmClient> nm_client_;
};

#endif
