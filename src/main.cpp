#include "WifiManager.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>

void print_usage(const char* program) {
    std::cout << "Uso: " << program << " [opciones] comando\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  scan                    Escanear redes disponibles\n";
    std::cout << "  list                    Listar redes guardadas\n";
    std::cout << "  connect <ssid>         Conectar a una red\n";
    std::cout << "  disconnect              Desconectar de la red actual\n";
    std::cout << "  remove <id>            Eliminar una red guardada\n";
    std::cout << "  status                  Mostrar estado de conexion\n";
    std::cout << "  monitor                 Modo interactivo con eventos\n";
    std::cout << "  password <id>           Ver contrasena de red guardada\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -i, --interface <iface> Interfaz WiFi (default: wlan0)\n";
    std::cout << "  -p, --password <pass>   Contrasena de la red\n";
    std::cout << "  -e, --enterprise        Usar autenticacion empresarial\n";
    std::cout << "  -h, --help              Mostrar esta ayuda\n";
}

std::string get_password() {
    std::string password;
    struct termios old_term, new_term;
    
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    
    std::cout << "Contrasena: ";
    std::getline(std::cin, password);
    std::cout << "\n";
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    
    return password;
}

void print_network(const Network& net, int index) {
    std::cout << std::setw(2) << index << " ";
    if (net.is_current) {
        std::cout << "*";
    } else {
        std::cout << " ";
    }
    std::cout << " " << std::setw(-32) << net.ssid;
    std::cout << " " << std::setw(4) << net.signal_strength << " dBm";
    std::cout << " [" << net.get_security_string() << "]";
    if (net.is_connected) {
        std::cout << " [CONECTADO]";
    }
    std::cout << "\n";
}

void print_saved_network(const SavedNetwork& net, int index) {
    std::cout << std::setw(2) << index << " ";
    std::cout << "ID: " << net.id << " | " << net.ssid;
    if (net.is_disabled()) {
        std::cout << " [DESHABILITADO]";
    }
    std::cout << "\n";
}

void print_status(const ConnectionStatus& status) {
    std::cout << "Estado: ";
    switch (status.state) {
        case ConnectionState::COMPLETED:
            std::cout << "Conectado";
            break;
        case ConnectionState::DISCONNECTED:
            std::cout << "Desconectado";
            break;
        case ConnectionState::SCANNING:
            std::cout << "Escaneando";
            break;
        case ConnectionState::AUTHENTICATING:
            std::cout << "Autenticando";
            break;
        case ConnectionState::ASSOCIATING:
            std::cout << "Asociando";
            break;
        case ConnectionState::HANDSHAKING:
            std::cout << "Handshake";
            break;
        default:
            std::cout << "Desconocido";
    }
    std::cout << "\n";
    
    if (!status.ssid.empty()) {
        std::cout << "SSID: " << status.ssid << "\n";
    }
    if (!status.bssid.empty()) {
        std::cout << "BSSID: " << status.bssid << "\n";
    }
    if (!status.ip_address.empty()) {
        std::cout << "IP: " << status.ip_address << "\n";
    }
    if (!status.security.empty()) {
        std::cout << "Seguridad: " << status.security << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string iface = "wlan0";
    std::string password;
    bool enterprise = false;
    std::string command;

    static struct option long_options[] = {
        {"interface", required_argument, 0, 'i'},
        {"password", required_argument, 0, 'p'},
        {"enterprise", no_argument, 0, 'e'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "i:p:eh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                iface = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'e':
                enterprise = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind < argc) {
        command = argv[optind];
    }

    WifiManager wm;
    
    if (!wm.initialize(iface)) {
        return 1;
    }

    if (command == "scan") {
        std::cout << "Escaneando redes...\n";
        if (wm.trigger_scan()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
        auto networks = wm.scan();
        
        if (networks.empty()) {
            std::cout << "No se encontraron redes.\n";
        } else {
            std::cout << "\nRedes disponibles:\n";
            std::cout << std::string(70, '-') << "\n";
            for (size_t i = 0; i < networks.size(); ++i) {
                print_network(networks[i], i + 1);
            }
        }
    }
    else if (command == "list") {
        auto networks = wm.list_networks();
        
        if (networks.empty()) {
            std::cout << "No hay redes guardadas.\n";
        } else {
            std::cout << "Redes guardadas:\n";
            std::cout << std::string(50, '-') << "\n";
            for (size_t i = 0; i < networks.size(); ++i) {
                print_saved_network(networks[i], i + 1);
            }
        }
    }
    else if (command == "connect") {
        if (optind + 1 >= argc) {
            std::cerr << "Error: SSID requerido\n";
            std::cerr << "Uso: " << argv[0] << " connect <ssid> [-p password]\n";
            return 1;
        }
        
        std::string ssid = argv[optind + 1];
        
        if (password.empty()) {
            password = get_password();
        }
        
        std::cout << "Conectando a '" << ssid << "'...\n";
        
        ConnectionConfig config;
        config.ssid = ssid;
        config.password = password;
        config.is_enterprise = enterprise;
        
        if (enterprise) {
            config.security_type = SecurityType::WPA_WPA2_ENTERPRISE;
            std::cout << "Modo empresarial habilitado\n";
        } else {
            config.security_type = SecurityType::WPA2_PSK;
        }
        
        auto result = wm.connect(config);
        
        if (result.has_value()) {
            std::cout << "Conectado exitosamente! (Network ID: " << result.value() << ")\n";
        } else {
            std::cerr << "Error al conectar\n";
        }
    }
    else if (command == "disconnect") {
        if (wm.disconnect()) {
            std::cout << "Desconectado\n";
        } else {
            std::cerr << "Error al desconectar\n";
        }
    }
    else if (command == "remove") {
        if (optind + 1 >= argc) {
            std::cerr << "Error: ID de red requerido\n";
            return 1;
        }
        
        std::string id = argv[optind + 1];
        
        if (wm.remove_network(id)) {
            std::cout << "Red eliminada\n";
        } else {
            std::cerr << "Error al eliminar red\n";
        }
    }
    else if (command == "status") {
        auto status = wm.get_status();
        print_status(status);
    }
    else if (command == "monitor") {
        std::cout << "Modo monitor - Presiona Ctrl+C para salir\n\n";
        
        wm.set_event_callback([](const std::string& event) {
            if (event.find("CTRL-EVENT-CONNECTED") != std::string::npos) {
                std::cout << "\n[INFO] Conectado a la red\n";
            } else if (event.find("CTRL-EVENT-DISCONNECTED") != std::string::npos) {
                std::cout << "\n[INFO] Desconectado de la red\n";
            } else if (event.find("CTRL-EVENT-SCAN-RESULTS") != std::string::npos) {
                std::cout << "\n[INFO] Resultados de escaneo disponibles\n";
            } else if (event.find("CTRL-EVENT-SCAN-FAILED") != std::string::npos) {
                std::cout << "\n[ERROR] Escaneo fallido\n";
            } else if (event.find("WPAS-") != std::string::npos || event.find("<3>") != std::string::npos) {
                std::string msg = event;
                size_t pos = msg.find("<3>");
                if (pos != std::string::npos) {
                    msg = msg.substr(pos + 3);
                }
                std::cout << "[EVENT] " << msg << "\n";
            }
        });
        
        wm.start_monitoring();
        
        while (true) {
            wm.process_events(1000);
        }
    }
    else if (command == "password") {
        if (optind + 1 >= argc) {
            std::cerr << "Error: ID de red requerido\n";
            return 1;
        }
        
        std::string id = argv[optind + 1];
        std::string psk = wm.get_password(id);
        
        if (!psk.empty()) {
            std::cout << "Contrasena (PSK): " << psk << "\n";
        } else {
            std::cout << "No se encontro contrasena para la red " << id << "\n";
        }
    }
    else if (command == "help" || command.empty()) {
        print_usage(argv[0]);
    }
    else {
        std::cerr << "Comando desconocido: " << command << "\n";
        print_usage(argv[0]);
        return 1;
    }

    wm.cleanup();
    return 0;
}
