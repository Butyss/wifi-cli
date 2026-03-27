# WiFi CLI

A lightweight WiFi management tool for Linux with an interactive terminal UI (TUI).

## Features

- **Interactive TUI**: User-friendly interface using ncurses with compact design
- **Network scanning**: Automatic WiFi network discovery every 7 seconds
- **Connection management**: Connect to open and secured networks
- **Ping display**: Real-time connection latency monitoring
- **Saved networks**: View stored network passwords
- **Network info panel**: Detailed information about each network
- **Forget networks**: Remove saved network configurations

## Requirements

- Linux
- NetworkManager
- ncurses
- GLib2 (for D-Bus communication)

## Installation

### Build from source

```bash
# Clone the repository
git clone https://github.com/yourusername/wifi-cli.git
cd wifi-cli

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Install (optional)
sudo make install
```

### Install dependencies (Arch Linux)

```bash
sudo pacman -S networkmanager ncurses glib2
```

### Install dependencies (Debian/Ubuntu)

```bash
sudo apt install libnm-dev libncurses-dev libglib2.0-dev
```

## Usage

### Run the application

```bash
./wifi-cli
# or after installation:
wifi-cli
```

### Controls

| Key | Action |
|-----|--------|
| `ENTER` | Connect to selected network |
| `R` | Manual network scan |
| `I` | Show network information panel |
| `F` | Forget (delete) selected network |
| `Q` | Quit application |
| `UP/DOWN` or `K/J` | Navigate network list |
| `ESC` | Cancel password input |

### Interface Layout

```
+------------------------------------------------+
| WiFi Conectado: SSID | XXms                     |
+------------------------------------------------+
| Redes:                                         |
|   > NetworkName      ####  -50dBm  [WPA2]     |
|     OtherNetwork    ####  -65dBm  [WPA3]     |
|                                                 |
+------------------------------------------------+
| ENTER:Conectar  R:Escanear  F:Olvidar  Q:Salir|
+------------------------------------------------+
```

### Network Information Panel

Press `I` on a selected network to view:
- SSID
- BSSID (MAC address)
- Signal strength (dBm)
- Frequency (MHz)
- Security type (WPA2-PSK, WPA3-SAE, etc.)

### Connecting to a Network

1. Navigate to the desired network using arrow keys
2. Press `ENTER`
3. For secured networks, enter the password
4. Press `ENTER` to connect
5. Wait for "Conectando..." message

### Forgetting a Network

1. Navigate to the saved network
2. Press `F` to delete the saved configuration
3. A confirmation message will appear

## Architecture

```
wifi-cli/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── docs/
│   └── API.md          # API documentation
└── src/
    ├── main.cpp        # Entry point
    ├── WifiManager.cpp # Core WiFi management logic
    ├── WifiManager.h
    ├── NmClient.cpp    # NetworkManager D-Bus client
    ├── NmClient.h
    ├── Network.cpp     # Network data models
    ├── Network.h
    ├── WifiTUI.cpp     # Terminal UI implementation
    └── WifiTUI.h
```

### Key Components

- **NmClient**: Handles D-Bus communication with NetworkManager for WiFi operations
- **WifiManager**: High-level WiFi operations (scan, connect, disconnect, get status)
- **WifiTUI**: Interactive terminal user interface with ncurses

## Troubleshooting

### "Error al conectar" when connecting

- Ensure the password is correct
- Check if the network uses a special security type (WPA3, Enterprise, etc.)
- Try forgetting and reconnecting to the network

### Network not found

- Move closer to the access point
- Try a manual scan with `R`
- Ensure NetworkManager is running: `systemctl status NetworkManager`

### Permission denied

- Some operations require elevated privileges
- NetworkManager should handle permissions automatically

## License

MIT License - See [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.
