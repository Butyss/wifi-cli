#ifndef NM_CLIENT_H
#define NM_CLIENT_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace NmCli {

struct AccessPoint {
    std::string path;
    std::string ssid;
    std::string bssid;
    int strength;
    uint32_t frequency;
    uint32_t flags;
    bool in_use;
};

struct WifiDevice {
    std::string path;
    std::string iface;
    bool connected;
    std::string connection_path;
    std::string connection_name;
};

struct SavedConnection {
    std::string path;
    std::string uuid;
    std::string id;
    std::string type;
    std::string device;
};

class NmClient {
public:
    NmClient();
    ~NmClient();

    bool init();
    void cleanup();

    std::optional<WifiDevice> get_wifi_device();
    std::vector<AccessPoint> get_access_points(bool scan_first = false);
    bool scan();

    std::vector<SavedConnection> get_saved_connections();
    bool connect(const std::string& ssid);
    bool connect_with_password(const std::string& ssid, const std::string& password);
    bool disconnect();

    std::optional<std::string> get_saved_password(const std::string& connection_id);

private:
    void* bus;
    bool initialized;

    std::string get_string_property(const std::string& service, const std::string& path,
                                    const std::string& iface, const std::string& prop);
    int get_int_property(const std::string& service, const std::string& path,
                         const std::string& iface, const std::string& prop);
    std::vector<std::string> get_object_paths(const std::string& service,
                                               const std::string& path,
                                               const std::string& iface,
                                               const std::string& method);
};

std::string bytes_to_string(const unsigned char* data, size_t len);

}

#endif
