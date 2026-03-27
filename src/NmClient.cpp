#include "NmClient.h"
#include <glib.h>
#include <gio/gio.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace NmCli {

static const char* NM_SERVICE = "org.freedesktop.NetworkManager";
static const char* NM_DEVICE_IFACE = "org.freedesktop.NetworkManager.Device";
static const char* NM_DEVICE_WIRELESS_IFACE = "org.freedesktop.NetworkManager.Device.Wireless";
static const char* NM_AP_IFACE = "org.freedesktop.NetworkManager.AccessPoint";
static const char* NM_SETTINGS_IFACE = "org.freedesktop.NetworkManager.Settings";
static const char* NM_SETTINGS_CONNECTION_IFACE = "org.freedesktop.NetworkManager.Settings.Connection";
static const char* NM_ACTIVE_IFACE = "org.freedesktop.NetworkManager.Connection.Active";

NmClient::NmClient() : bus(nullptr), initialized(false) {}

NmClient::~NmClient() {
    cleanup();
}

bool NmClient::init() {
    if (initialized) return true;

    GError* error = nullptr;
    bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (error) {
        g_error_free(error);
        return false;
    }
    initialized = true;
    return true;
}

void NmClient::cleanup() {
    if (bus) {
        g_object_unref(G_OBJECT(bus));
        bus = nullptr;
    }
    initialized = false;
}

std::string NmClient::get_string_property(const std::string& service, const std::string& path,
                                          const std::string& iface, const std::string& prop) {
    if (!bus) return "";

    GError* error = nullptr;
    GDBusConnection* conn = G_DBUS_CONNECTION(bus);
    
    GVariant* result = g_dbus_connection_call_sync(conn, service.c_str(), path.c_str(),
                                                    "org.freedesktop.DBus.Properties", "Get",
                                                    g_variant_new("(ss)", iface.c_str(), prop.c_str()),
                                                    nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) {
        g_error_free(error);
        return "";
    }

    if (!result) return "";

    GVariant* value = nullptr;
    g_variant_get(result, "(v)", &value);
    g_variant_unref(result);

    if (!value) return "";

    const char* str = g_variant_get_string(value, nullptr);
    std::string ret = str ? str : "";
    g_variant_unref(value);
    return ret;
}

int NmClient::get_int_property(const std::string& service, const std::string& path,
                               const std::string& iface, const std::string& prop) {
    if (!bus) return 0;

    GError* error = nullptr;
    GDBusConnection* conn = G_DBUS_CONNECTION(bus);
    
    GVariant* result = g_dbus_connection_call_sync(conn, service.c_str(), path.c_str(),
                                                    "org.freedesktop.DBus.Properties", "Get",
                                                    g_variant_new("(ss)", iface.c_str(), prop.c_str()),
                                                    nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) {
        g_error_free(error);
        return 0;
    }

    if (!result) return 0;

    GVariant* value = nullptr;
    g_variant_get(result, "(v)", &value);
    g_variant_unref(result);

    if (!value) return 0;

    int ret = 0;
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_BYTE)) {
        ret = g_variant_get_byte(value);
    } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT16)) {
        ret = g_variant_get_int16(value);
    } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT16)) {
        ret = g_variant_get_uint16(value);
    } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT32)) {
        ret = g_variant_get_int32(value);
    } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32)) {
        ret = g_variant_get_uint32(value);
    } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
        ret = g_variant_get_boolean(value);
    }
    g_variant_unref(value);
    return ret;
}

std::vector<std::string> NmClient::get_object_paths(const std::string& service,
                                                     const std::string& path,
                                                     const std::string& iface,
                                                     const std::string& method) {
    std::vector<std::string> result;
    if (!bus) return result;

    GError* error = nullptr;
    GDBusConnection* conn = G_DBUS_CONNECTION(bus);
    
    GVariant* call_result = g_dbus_connection_call_sync(conn, service.c_str(), path.c_str(),
                                                         iface.c_str(), method.c_str(),
                                                         nullptr, nullptr,
                                                         G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) {
        g_error_free(error);
        return result;
    }

    if (!call_result) return result;

    GVariantIter* iter = nullptr;
    g_variant_get(call_result, "(ao)", &iter);
    g_variant_unref(call_result);

    if (!iter) return result;

    const char* obj_path;
    while (g_variant_iter_loop(iter, "o", &obj_path)) {
        result.push_back(obj_path);
    }
    g_variant_iter_free(iter);

    return result;
}

std::optional<WifiDevice> NmClient::get_wifi_device() {
    if (!initialized) {
        if (!init()) return std::nullopt;
    }

    auto devices = get_object_paths(NM_SERVICE, "/org/freedesktop/NetworkManager",
                                    NM_SERVICE, "GetDevices");
    if (devices.empty()) return std::nullopt;

    for (const auto& dev_path : devices) {
        int dev_type = get_int_property(NM_SERVICE, dev_path, NM_DEVICE_IFACE, "DeviceType");
        if (dev_type == 2) {
            WifiDevice dev;
            dev.path = dev_path;
            dev.iface = get_string_property(NM_SERVICE, dev_path, NM_DEVICE_IFACE, "Interface");
            dev.connected = get_int_property(NM_SERVICE, dev_path, NM_DEVICE_IFACE, "State") == 100;
            
            auto active_conns = get_object_paths(NM_SERVICE, "/org/freedesktop/NetworkManager",
                                                 NM_SERVICE, "ActiveConnections");
            for (const auto& ac_path : active_conns) {
                std::string ac_dev = get_string_property(NM_SERVICE, ac_path, NM_ACTIVE_IFACE, "Devices");
                std::string ac_id = get_string_property(NM_SERVICE, ac_path, NM_ACTIVE_IFACE, "Id");
                
                if (ac_dev.find(dev.iface) != std::string::npos && !ac_id.empty()) {
                    dev.connection_path = ac_path;
                    dev.connection_name = ac_id;
                    break;
                }
            }
            
            return dev;
        }
    }
    return std::nullopt;
}

bool NmClient::scan() {
    auto dev = get_wifi_device();
    if (!dev) return false;

    GError* error = nullptr;
    GDBusConnection* conn = G_DBUS_CONNECTION(bus);
    
    GVariant* result = g_dbus_connection_call_sync(conn, NM_SERVICE, dev->path.c_str(),
                                                    NM_DEVICE_WIRELESS_IFACE, "RequestScan",
                                                    nullptr, nullptr,
                                                    G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) {
        g_error_free(error);
        return false;
    }
    
    if (result) g_variant_unref(result);
    return true;
}

std::vector<AccessPoint> NmClient::get_access_points(bool scan_first) {
    std::vector<AccessPoint> result;
    if (!initialized) {
        if (!init()) return result;
    }

    if (scan_first) {
        scan();
    }

    auto dev = get_wifi_device();
    if (!dev) return result;

    auto ap_paths = get_object_paths(NM_SERVICE, dev->path, NM_DEVICE_WIRELESS_IFACE, "GetAccessPoints");
    if (ap_paths.empty()) return result;

    for (const auto& ap_path : ap_paths) {
        AccessPoint ap;
        ap.path = ap_path;

        GVariant* ssid_var = nullptr;
        GError* error = nullptr;
        GDBusConnection* conn = G_DBUS_CONNECTION(bus);
        
        GVariant* prop_result = g_dbus_connection_call_sync(conn, NM_SERVICE, ap_path.c_str(),
                                                             "org.freedesktop.DBus.Properties", "Get",
                                                             g_variant_new("(ss)", NM_AP_IFACE, "Ssid"),
                                                             nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
        if (!error && prop_result) {
            g_variant_get(prop_result, "(v)", &ssid_var);
            g_variant_unref(prop_result);
        }

        if (ssid_var) {
            gsize len = 0;
            const uint8_t* data = static_cast<const uint8_t*>(g_variant_get_data(ssid_var));
            if (data && g_variant_get_size(ssid_var) > 0) {
                len = g_variant_get_size(ssid_var);
                ap.ssid = bytes_to_string(data, len);
            }
            g_variant_unref(ssid_var);
        }

        ap.strength = get_int_property(NM_SERVICE, ap_path, NM_AP_IFACE, "Strength");
        ap.frequency = get_int_property(NM_SERVICE, ap_path, NM_AP_IFACE, "Frequency");
        ap.flags = get_int_property(NM_SERVICE, ap_path, NM_AP_IFACE, "Flags");
        ap.bssid = get_string_property(NM_SERVICE, ap_path, NM_AP_IFACE, "HwAddress");
        ap.in_use = get_int_property(NM_SERVICE, ap_path, NM_AP_IFACE, "LastSeen") > 0;

        if (!ap.ssid.empty()) {
            result.push_back(ap);
        }
    }

    return result;
}

std::vector<SavedConnection> NmClient::get_saved_connections() {
    std::vector<SavedConnection> result;
    if (!initialized) {
        if (!init()) return result;
    }

    auto conn_paths = get_object_paths(NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
                                       NM_SETTINGS_IFACE, "ListConnections");
    
    for (const auto& conn_path : conn_paths) {
        SavedConnection conn;
        conn.path = conn_path;
        conn.uuid = get_string_property(NM_SERVICE, conn_path, NM_SETTINGS_CONNECTION_IFACE, "uuid");
        conn.id = get_string_property(NM_SERVICE, conn_path, NM_SETTINGS_CONNECTION_IFACE, "id");
        conn.type = get_string_property(NM_SERVICE, conn_path, NM_SETTINGS_CONNECTION_IFACE, "type");
        
        if (conn.type == "802-11-wireless") {
            result.push_back(conn);
        }
    }

    return result;
}

bool NmClient::connect(const std::string& ssid) {
    if (!initialized) {
        if (!init()) return false;
    }

    auto saved = get_saved_connections();
    for (const auto& c : saved) {
        if (c.id == ssid) {
            GError* error = nullptr;
            GDBusConnection* conn = G_DBUS_CONNECTION(bus);
            
            GVariant* result = g_dbus_connection_call_sync(conn, NM_SERVICE, "/org/freedesktop/NetworkManager",
                                                            NM_SERVICE, "ActivateConnection",
                                                            g_variant_new("(oo)", 
                                                                          c.path.c_str(),
                                                                          "/"),
                                                            nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
            if (error) {
                g_error_free(error);
                return false;
            }
            if (result) g_variant_unref(result);
            return true;
        }
    }
    return false;
}

bool NmClient::connect_with_password(const std::string& ssid, const std::string& password) {
    if (!initialized) {
        if (!init()) return false;
    }

    auto aps = get_access_points(false);
    for (const auto& ap : aps) {
        if (ap.ssid == ssid) {
            if (fork() == 0) {
                if (fork() == 0) {
                    int fd = open("/dev/null", O_RDWR);
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                    if (fd > 2) close(fd);
                    setsid();
                    execlp("nmcli", "nmcli", "d", "wifi", "connect", ssid.c_str(), 
                           "password", password.c_str(), "ifname", "wlan0", (char*)nullptr);
                    _exit(1);
                }
                _exit(0);
            }
            wait(nullptr);
            return true;
        }
    }
    return false;
}

bool NmClient::disconnect() {
    if (!initialized) {
        if (!init()) return false;
    }

    auto dev = get_wifi_device();
    if (!dev) return false;

    GError* error = nullptr;
    GDBusConnection* conn = G_DBUS_CONNECTION(bus);
    
    GVariant* result = g_dbus_connection_call_sync(conn, NM_SERVICE, "/org/freedesktop/NetworkManager",
                                                    NM_SERVICE, "DeactivateConnection",
                                                    g_variant_new("(o)", dev->connection_path.c_str()),
                                                    nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) {
        g_error_free(error);
        return false;
    }
    
    if (result) g_variant_unref(result);
    return true;
}

std::optional<std::string> NmClient::get_saved_password(const std::string& connection_id) {
    if (!initialized) {
        if (!init()) return std::nullopt;
    }

    auto saved = get_saved_connections();
    for (const auto& c : saved) {
        if (c.id == connection_id) {
            GError* error = nullptr;
            GDBusConnection* conn = G_DBUS_CONNECTION(bus);
            
            GVariant* result = g_dbus_connection_call_sync(conn, NM_SERVICE, c.path.c_str(),
                                                            NM_SETTINGS_CONNECTION_IFACE, "GetSecrets",
                                                            g_variant_new("(s)", "802-11-wireless-security"),
                                                            nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
            if (error) {
                g_error_free(error);
                return std::nullopt;
            }

            if (!result) return std::nullopt;

            const char* psk = nullptr;
            GVariantIter* iter = nullptr;
            g_variant_get(result, "(a{sa{sv}})", &iter);
            g_variant_unref(result);

            if (!iter) return std::nullopt;

            char* key = nullptr;
            GVariant* val = nullptr;
            while (g_variant_iter_loop(iter, "{sa{sv}}", &key, &val)) {
                if (strcmp(key, "802-11-wireless-security") == 0) {
                    GVariant* psk_var = g_variant_lookup_value(val, "psk", G_VARIANT_TYPE_STRING);
                    if (psk_var) {
                        psk = g_variant_get_string(psk_var, nullptr);
                        g_variant_unref(psk_var);
                    }
                }
            }
            g_variant_iter_free(iter);

            if (psk) {
                return std::string(psk);
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::string bytes_to_string(const unsigned char* data, size_t len) {
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len && data[i] != '\0'; ++i) {
        if (data[i] >= 32 && data[i] < 127) {
            result += static_cast<char>(data[i]);
        }
    }
    return result;
}

}
