# WiFi CLI - API Documentation

## Table of Contents

1. [Network.h](#networkh)
2. [WifiManager.h](#wifimanagerh)
3. [NmClient.h](#nmclienth)
4. [WifiTUI.h](#wifituih)

---

## Network.h

### Enums

#### `SecurityType`

WiFi security protocols.

```cpp
enum class SecurityType {
    OPEN,           // No encryption
    WPA2_PSK,       // WPA2 Personal
    WPA_WPA2_ENTERPRISE, // WPA/WPA2 Enterprise
    WPA3_SAE,       // WPA3 SAE (Simultaneous Authentication of Equals)
    WPA3_ENTERPRISE // WPA3 Enterprise
};
```

#### `ConnectionState`

Current WiFi connection state.

```cpp
enum class ConnectionState {
    DISCONNECTED,   // Not connected
    SCANNING,       // Searching for networks
    AUTHENTICATING, // Authenticating with network
    ASSOCIATING,    // Associating with access point
    ASSOCIATED,     // Associated with AP
    HANDSHAKING,    // 4-way handshake in progress
    COMPLETED,      // Successfully connected
    INACTIVE,       // Connection inactive
    UNKNOWN         // Unknown state
};
```

### Structs

#### `Network`

Represents a WiFi network discovered during scanning.

| Field | Type | Description |
|-------|------|-------------|
| `ssid` | `std::string` | Network name |
| `bssid` | `std::string` | Access point MAC address |
| `signal_strength` | `int` | Signal strength in dBm |
| `frequency` | `int` | Frequency in MHz |
| `security_types` | `std::vector<SecurityType>` | Supported security types |
| `id` | `std::string` | Network identifier (usually BSSID) |
| `is_connected` | `bool` | Currently connected flag |
| `is_current` | `bool` | Currently selected/highlighted |

**Example:**
```cpp
Network net;
net.ssid = "MyNetwork";
net.bssid = "AA:BB:CC:DD:EE:FF";
net.signal_strength = -45;
net.frequency = 5180;
net.security_types = {SecurityType::WPA2_PSK};
```

#### `SavedNetwork`

Represents a saved network configuration.

| Field | Type | Description |
|-------|------|-------------|
| `id` | `std::string` | Network UUID |
| `ssid` | `std::string` | Network name |

#### `ConnectionConfig`

Configuration for connecting to a network.

| Field | Type | Description |
|-------|------|-------------|
| `ssid` | `std::string` | Network name |
| `password` | `std::string` | Network password |
| `security_type` | `SecurityType` | Security protocol to use |
| `is_enterprise` | `bool` | Enterprise network flag |

#### `ConnectionStatus`

Current connection status information.

| Field | Type | Description |
|-------|------|-------------|
| `state` | `ConnectionState` | Connection state |
| `ssid` | `std::string` | Connected network name |
| `bssid` | `std::string` | Connected AP MAC |
| `ip_address` | `std::string` | Assigned IP address |
| `security` | `std::string` | Security protocol string |
| `signal` | `int` | Signal strength |
| `ping` | `int` | Connection latency in ms |
| `raw` | `std::string` | Raw status output |

### Functions

#### `parse_security_type`

```cpp
SecurityType parse_security_type(const std::string& flags);
```

Parses security flags string to SecurityType enum.

**Parameters:**
- `flags` - Security flags string from nmcli

**Returns:** Corresponding `SecurityType` value

---

## WifiManager.h

Singleton class for WiFi management operations.

### Methods

#### `get_instance()`

```cpp
static WifiManager& get_instance();
```

Returns the singleton instance of WifiManager.

**Returns:** Reference to the WifiManager singleton

---

#### `initialize()`

```cpp
bool initialize();
```

Initializes the WifiManager and connects to NetworkManager via D-Bus.

**Returns:** `true` if initialization successful, `false` otherwise

---

#### `cleanup()`

```cpp
void cleanup();
```

Cleans up resources and disconnects from NetworkManager.

---

#### `scan()`

```cpp
std::vector<Network> scan();
```

Performs a WiFi scan and returns discovered networks sorted by signal strength.

**Returns:** Vector of discovered `Network` objects

---

#### `trigger_scan()`

```cpp
bool trigger_scan();
```

Triggers an immediate network scan without returning results.

**Returns:** `true` if scan triggered successfully

---

#### `list_networks()`

```cpp
std::vector<SavedNetwork> list_networks();
```

Returns list of saved network configurations.

**Returns:** Vector of saved networks

---

#### `connect()`

```cpp
std::optional<int> connect(const ConnectionConfig& config);
```

Connects to a network using the provided configuration.

**Parameters:**
- `config` - Connection configuration

**Returns:** Optional integer (0 on success) or nullopt on failure

---

#### `disconnect()`

```cpp
bool disconnect();
```

Disconnects from the current network.

**Returns:** `true` if disconnect successful

---

#### `remove_network()`

```cpp
bool remove_network(const std::string& network_id);
```

Removes a saved network configuration.

**Parameters:**
- `network_id` - UUID of the network to remove

**Returns:** `true` if removal successful

---

#### `get_status()`

```cpp
ConnectionStatus get_status();
```

Gets the current connection status.

**Returns:** `ConnectionStatus` struct with current state

---

#### `get_current_connection_name()`

```cpp
std::string get_current_connection_name();
```

Gets the name of the currently connected network.

**Returns:** SSID of connected network, or empty string if not connected

---

#### `get_ping()`

```cpp
int get_ping();
```

Measures connection latency by pinging 8.8.8.8.

**Returns:** Ping in milliseconds, or -1 if failed

---

#### `get_password()`

```cpp
std::string get_password(const std::string& network_id);
```

Retrieves the stored password for a saved network.

**Parameters:**
- `network_id` - UUID of the network

**Returns:** Stored password string, or empty if not found

---

#### `is_connected_to_nm()`

```cpp
bool is_connected_to_nm() const;
```

Checks if connected to NetworkManager.

**Returns:** `true` if connected

---

## NmClient.h

D-Bus client for NetworkManager communication. Used internally by WifiManager.

### Structs

#### `WifiDevice`

Represents a WiFi network device.

| Field | Type | Description |
|-------|------|-------------|
| `path` | `std::string` | D-Bus object path |
| `iface` | `std::string` | Interface name (e.g., wlan0) |
| `connected` | `bool` | Connection status |
| `connection_path` | `std::string` | Active connection path |
| `connection_name` | `std::string` | Connected network name |

#### `AccessPoint`

Represents a WiFi access point.

| Field | Type | Description |
|-------|------|-------------|
| `path` | `std::string` | D-Bus object path |
| `ssid` | `std::string` | Network name |
| `bssid` | `std::string` | MAC address |
| `strength` | `int` | Signal strength (0-100) |
| `frequency` | `int` | Frequency in MHz |
| `flags` | `uint32_t` | Security flags |
| `in_use` | `bool` | Currently in use |

#### `SavedConnection`

Represents a saved NetworkManager connection.

| Field | Type | Description |
|-------|------|-------------|
| `path` | `std::string` | D-Bus object path |
| `uuid` | `std::string` | Connection UUID |
| `id` | `std::string` | Connection name (SSID) |
| `type` | `std::string` | Connection type |

### NmClient Class

```cpp
namespace NmCli {
    class NmClient {
    public:
        NmClient();
        ~NmClient();
        
        bool init();
        void cleanup();
        
        // Device operations
        std::optional<WifiDevice> get_wifi_device();
        bool scan();
        
        // Network operations
        std::vector<AccessPoint> get_access_points(bool scan_first = true);
        std::vector<SavedConnection> get_saved_connections();
        
        // Connection operations
        bool connect(const std::string& ssid);
        bool connect_with_password(const std::string& ssid, const std::string& password);
        bool disconnect();
        
        // Secrets
        std::optional<std::string> get_saved_password(const std::string& connection_id);
    };
}
```

---

## WifiTUI.h

Interactive terminal UI using ncurses.

### Enums

#### `TUIState`

Current UI state.

```cpp
enum class TUIState {
    NETWORKS_LIST,   // Main network list view
    PASSWORD_INPUT,   // Password entry dialog
    CONNECTING,       // Connection in progress
    NETWORK_INFO,    // Network information panel
    ERROR            // Error display
};
```

#### `TUIEvent`

Events returned from input handling.

```cpp
enum class TUIEvent {
    NONE,  // No event
    QUIT   // Quit application
};
```

### Structs

#### `WifiTUIConfig`

UI configuration options.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `auto_scan` | `bool` | `true` | Enable automatic scanning |
| `scan_interval_ms` | `int` | `7000` | Scan interval in milliseconds |
| `show_hidden` | `bool` | `false` | Show hidden networks |
| `color_output` | `bool` | `true` | Enable colored output |

### WifiTUI Class

```cpp
class WifiTUI {
public:
    WifiTUI();
    ~WifiTUI();
    
    bool init();
    void run();
    void stop();
    
    void set_config(const WifiTUIConfig& config);
    WifiTUIConfig get_config() const;

private:
    // Rendering methods
    void render();
    void render_header();       // Header with title and status
    void render_networks();     // Network list view
    void render_password_input(); // Password dialog
    void render_connecting();   // Connecting spinner
    void render_network_info(); // Info panel
    void render_error();        // Error display
    void render_border();       // UI borders
    
    // Drawing helpers
    void draw_signal_bars(int signal, int col);
    void draw_network_row(const Network& net, int row, bool selected);
    std::string truncate(const std::string& str, size_t width);
    
    // Input handling
    TUIEvent handle_input(int ch);
    void handle_resize();
};
```

### Methods

#### `init()`

```cpp
bool init();
```

Initializes ncurses and sets up the terminal.

**Returns:** `true` if initialization successful

---

#### `run()`

```cpp
void run();
```

Main event loop. Handles rendering and input until quit.

---

#### `stop()`

```cpp
void stop();
```

Stops the event loop and cleans up ncurses.

---

#### `set_config()`

```cpp
void set_config(const WifiTUIConfig& config);
```

Sets UI configuration options.

---

#### `get_config()`

```cpp
WifiTUIConfig get_config() const;
```

Gets current UI configuration.

---

### Private Methods

#### `render_header()`

Draws the header bar with:
- Title ("WiFi")
- Connection status ("Conectado: SSID | XXms" or "Desconectado")
- Top border

#### `render_networks()`

Draws the main network list:
- "Redes:" label
- Network rows with: selector, SSID, signal bars, dBm, security type
- Automatic scan trigger every 7 seconds

#### `draw_network_row()`

Draws a single network row with:
- Selection indicator (when selected)
- Truncated SSID (max 15 chars)
- Signal strength bars (#### for good, fewer for weak)
- Signal in dBm
- Security type in brackets
- [CONECTADO] badge when connected

---

## Usage Examples

### Connecting to a Network

```cpp
WifiManager& wm = WifiManager::get_instance();
wm.initialize();

ConnectionConfig config;
config.ssid = "MyNetwork";
config.password = "secretpassword";
config.security_type = SecurityType::WPA2_PSK;
config.is_enterprise = false;

auto result = wm.connect(config);
if (result) {
    std::cout << "Connected successfully!" << std::endl;
}
```

### Getting Connection Status

```cpp
ConnectionStatus status = wm.get_status();

if (status.state == ConnectionState::COMPLETED) {
    std::cout << "Connected to: " << status.ssid << std::endl;
    std::cout << "Signal: " << status.signal << " dBm" << std::endl;
    
    int ping = wm.get_ping();
    if (ping >= 0) {
        std::cout << "Latency: " << ping << "ms" << std::endl;
    }
}
```

### Scanning Networks

```cpp
wm.trigger_scan();
std::vector<Network> networks = wm.scan();

for (const auto& net : networks) {
    std::cout << net.ssid << " (" << net.signal_strength << " dBm)" << std::endl;
}
```
