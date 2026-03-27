# WiFi CLI

A lightweight WiFi management tool for Linux with an interactive terminal UI (TUI).

## Features

- **Interactive TUI**: User-friendly interface using ncurses
- **Network scanning**: Automatic WiFi network discovery every 7 seconds
- **Connection management**: Connect to open and secured networks
- **Saved networks**: View stored network passwords
- **Network info panel**: Detailed information about each network
- **Forget networks**: Remove saved network configurations

## Requirements

- Linux (tested on Arch Linux)
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

### Network Information Panel

Press `I` on a selected network to view:
- SSID
- BSSID (MAC address)
- Signal strength (dBm)
- Frequency (MHz)
- Security type (WPA2-PSK, WPA3-SAE, etc.)
- Saved status
- Stored password (if saved)

### Connecting to a Network

1. Navigate to the desired network using arrow keys
2. Press `ENTER`
3. For secured networks, enter the password
4. Press `ENTER` to connect

### Forgetting a Network

1. Navigate to the saved network
2. Press `F` to delete the saved configuration

## Architecture

```
wifi-cli/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp          # Entry point
    ├── WifiManager.cpp   # Core WiFi management logic
    ├── WifiManager.h
    ├── NmClient.cpp     # NetworkManager D-Bus client
    ├── NmClient.h
    ├── Network.cpp       # Network data models
    ├── Network.h
    ├── WifiTUI.cpp       # Terminal UI implementation
    └── WifiTUI.h
```

### Key Components

- **NmClient**: Handles D-Bus communication with NetworkManager
- **WifiManager**: High-level WiFi operations (scan, connect, disconnect)
- **WifiTUI**: Interactive terminal user interface

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

MIT License

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.
