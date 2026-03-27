#include "WifiTUI.h"
#include <ncurses.h>
#include <algorithm>
#include <chrono>

WifiTUI::WifiTUI()
    : wm_(WifiManager::get_instance())
    , state_(TUIState::NETWORKS_LIST)
    , prev_state_(TUIState::NETWORKS_LIST)
    , selected_index_(0)
    , scroll_offset_(0)
    , screen_rows_(24)
    , screen_cols_(80)
    , running_(false)
    , scan_msg_frames_(0)
    , last_scan_time_(std::chrono::steady_clock::now())
    , status_message_()
    , status_msg_frames_(0)
    , connecting_frames_(0)
{
    config_.auto_scan = true;
    config_.scan_interval_ms = 7000;
    config_.show_hidden = false;
    config_.color_output = true;
}

WifiTUI::~WifiTUI() {
    stop();
}

bool WifiTUI::init() {
    WINDOW* win = initscr();
    if (win == nullptr) {
        return false;
    }
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLUE);
    }
    
    getmaxyx(stdscr, screen_rows_, screen_cols_);
    
    erase();
    refresh();
    
    return true;
}

void WifiTUI::run() {
    running_ = true;
    
    networks_ = wm_.scan();
    last_scan_time_ = std::chrono::steady_clock::now();
    connecting_frames_ = 0;
    
    timeout(100);
    
    int ch;
    while (running_) {
        render();
        
        ch = getch();
        
        if (ch != ERR) {
            TUIEvent event = handle_input(ch);
            if (event == TUIEvent::QUIT) {
                break;
            }
        }
        
        if (state_ == TUIState::CONNECTING) {
            connecting_frames_++;
            if (connecting_frames_ > 30) {
                networks_ = wm_.scan();
                state_ = TUIState::NETWORKS_LIST;
                connecting_frames_ = 0;
            }
        }
        
        handle_resize();
    }
    
    stop();
}

void WifiTUI::stop() {
    running_ = false;
    endwin();
}

void WifiTUI::set_config(const WifiTUIConfig& config) {
    config_ = config;
}

WifiTUIConfig WifiTUI::get_config() const {
    return config_;
}

void WifiTUI::render() {
    erase();
    
    switch (state_) {
        case TUIState::NETWORKS_LIST:
            render_networks();
            break;
        case TUIState::PASSWORD_INPUT:
            render_password_input();
            break;
        case TUIState::CONNECTING:
            render_connecting();
            break;
        case TUIState::NETWORK_INFO:
            render_networks();
            render_network_info();
            break;
        case TUIState::ERROR:
            render_error();
            break;
        default:
            render_networks();
            break;
    }
    
    refresh();
}

void WifiTUI::render_header() {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, 0, "[ WiFi CLI ]");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    ConnectionStatus status = wm_.get_status();
    std::string status_str;

    if (status.state == ConnectionState::COMPLETED) {
        status_str = "Conectado: " + status.ssid;
    } else {
        status_str = "Desconectado";
        attron(COLOR_PAIR(3));
    }
    
    int status_pos = screen_cols_ - status_str.length() - 3;
    if (status_pos > 0) {
        mvprintw(0, status_pos, "[%s]", status_str.c_str());
    }
    attroff(COLOR_PAIR(1) | COLOR_PAIR(3));
    
    mvaddstr(1, 0, "+");
    for (int i = 1; i < screen_cols_ - 1; i++) {
        addch('-');
    }
    addch('+');
}

void WifiTUI::render_networks() {
    render_header();
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_scan_time_).count();
    
    if (elapsed >= 7000) {
        scan_msg_frames_ = 3;
        last_scan_time_ = now;
        wm_.trigger_scan();
        networks_ = wm_.scan();
    }
    
    if (scan_msg_frames_ > 0) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(2, 35, " [Escaneando...] ");
        attroff(COLOR_PAIR(2) | A_BOLD);
        scan_msg_frames_--;
    }
    
    if (status_msg_frames_ > 0) {
        int msg_len = status_message_.length() + 4;
        int box_y = screen_rows_ / 2 - 1;
        int box_x = screen_cols_ / 2 - msg_len / 2;
        
        attron(COLOR_PAIR(1));
        mvaddch(box_y, box_x, '+');
        for (int i = 1; i < msg_len - 1; i++) mvaddch(box_y, box_x + i, '-');
        mvaddch(box_y, box_x + msg_len - 1, '+');
        
        for (int y = box_y + 1; y < box_y + 3; y++) {
            mvaddch(y, box_x, '|');
            for (int i = 1; i < msg_len - 1; i++) mvaddch(y, box_x + i, ' ');
            mvaddch(y, box_x + msg_len - 1, '|');
        }
        
        mvaddch(box_y + 3, box_x, '+');
        for (int i = 1; i < msg_len - 1; i++) mvaddch(box_y + 3, box_x + i, '-');
        mvaddch(box_y + 3, box_x + msg_len - 1, '+');
        attroff(COLOR_PAIR(1));
        
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(box_y + 1, box_x + 2, "%s", status_message_.c_str());
        attroff(COLOR_PAIR(2) | A_BOLD);
        
        status_msg_frames_--;
    }
    
    mvprintw(2, 2, "Redes disponibles:");
    
    if (networks_.empty()) {
        attron(A_BOLD);
        mvprintw(screen_rows_ / 2, screen_cols_ / 2 - 10, "Buscando redes...");
        attroff(A_BOLD);
        render_status_bar();
        return;
    }
    
    int start_row = 4;
    int visible_count = screen_rows_ - 7;
    
    for (int i = 0; i < visible_count && i < (int)networks_.size(); ++i) {
        int net_idx = i + scroll_offset_;
        if (net_idx >= (int)networks_.size()) break;
        
        const Network& net = networks_[net_idx];
        draw_network_row(net, start_row + i, net_idx == selected_index_);
    }
    
    render_status_bar();
}

void WifiTUI::render_status_bar() {
    int bar_row = screen_rows_ - 2;
    
    mvaddstr(bar_row, 0, "+");
    for (int i = 1; i < screen_cols_ - 1; i++) {
        addch('-');
    }
    addch('+');
    
    attron(A_REVERSE);
    mvprintw(bar_row, 2, "ENTER");
    mvprintw(bar_row, 9, "R:Escanear");
    mvprintw(bar_row, 23, "I:Info");
    mvprintw(bar_row, 33, "F:Forget");
    mvprintw(bar_row, 45, "Q:Salir");
    mvprintw(bar_row, screen_cols_ - 12, "UP/DOWN");
    attroff(A_REVERSE);
}

void WifiTUI::render_password_input() {
    render_header();
    
    mvprintw(screen_rows_ / 2 - 2, 2, "Conectar a: %s", pending_network_.ssid.c_str());
    mvprintw(screen_rows_ / 2, 2, "Contrasena: %s", std::string(password_input_.length(), '*').c_str());
    mvprintw(screen_rows_ / 2 + 1, 2, "Presiona ENTER para conectar, ESC para cancelar");
    
    render_status_bar();
}

void WifiTUI::render_connecting() {
    render_header();
    
    attron(A_BOLD);
    mvprintw(screen_rows_ / 2, screen_cols_ / 2 - 10, "Conectando...");
    attroff(A_BOLD);
    
    render_status_bar();
}

void WifiTUI::render_error() {
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(screen_rows_ / 2 - 1, screen_cols_ / 2 - error_message_.length() / 2, "%s", error_message_.c_str());
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    attron(A_REVERSE);
    mvprintw(screen_rows_ / 2 + 1, screen_cols_ / 2 - 20, "Presiona cualquier tecla para continuar...");
    attroff(A_REVERSE);
}

void WifiTUI::render_network_info() {
    int box_width = 45;
    int box_height = 11;
    int start_y = (screen_rows_ - box_height) / 2;
    int start_x = (screen_cols_ - box_width) / 2;
    
    attron(COLOR_PAIR(5));
    for (int y = start_y; y < start_y + box_height; y++) {
        for (int x = start_x; x < start_x + box_width; x++) {
            mvaddch(y, x, ' ');
        }
    }
    
    mvaddch(start_y, start_x, '+');
    mvaddch(start_y, start_x + box_width - 1, '+');
    mvaddch(start_y + box_height - 1, start_x, '+');
    mvaddch(start_y + box_height - 1, start_x + box_width - 1, '+');
    for (int x = start_x + 1; x < start_x + box_width - 1; x++) {
        mvaddch(start_y, x, '-');
        mvaddch(start_y + box_height - 1, x, '-');
    }
    for (int y = start_y + 1; y < start_y + box_height - 1; y++) {
        mvaddch(y, start_x, '|');
        mvaddch(y, start_x + box_width - 1, '|');
    }
    
    attron(A_BOLD);
    mvprintw(start_y, start_x + 2, "[ Informacion de Red ]");
    attroff(A_BOLD);
    
    mvprintw(start_y + 2, start_x + 2, "SSID: %s", info_network_.ssid.c_str());
    mvprintw(start_y + 3, start_x + 2, "BSSID: %s", info_network_.bssid.c_str());
    mvprintw(start_y + 4, start_x + 2, "Senal: %d dBm", info_network_.signal_strength);
    mvprintw(start_y + 5, start_x + 2, "Frecuencia: %d MHz", info_network_.frequency);
    mvprintw(start_y + 6, start_x + 2, "Seguridad: %s", info_network_.get_security_string().c_str());
    mvprintw(start_y + 7, start_x + 2, "Guardada: %s", info_saved_ ? "Si" : "No");
    if (info_saved_ && !info_password_.empty()) {
        attron(A_BOLD);
        mvprintw(start_y + 8, start_x + 2, "Password: %s", info_password_.c_str());
        attroff(A_BOLD);
    }
    
    attroff(COLOR_PAIR(5));
    
    attron(A_REVERSE);
    mvprintw(start_y + box_height - 2, start_x + 2, "Q/I: Cerrar");
    attroff(A_REVERSE);
}

TUIEvent WifiTUI::handle_input(int ch) {
    switch (state_) {
        case TUIState::NETWORKS_LIST: {
            switch (ch) {
                case KEY_UP:
                case 'k':
                case 'K':
                    if (selected_index_ > 0) {
                        selected_index_--;
                        if (selected_index_ < scroll_offset_) {
                            scroll_offset_ = selected_index_;
                        }
                    }
                    break;
                    
                case KEY_DOWN:
                case 'j':
                case 'J':
                    if (selected_index_ < (int)networks_.size() - 1) {
                        selected_index_++;
                        int max_visible = screen_rows_ - 7;
                        if (selected_index_ >= scroll_offset_ + max_visible) {
                            scroll_offset_++;
                        }
                    }
                    break;
                    
                case '\n':
                case KEY_ENTER: {
                    if (selected_index_ < (int)networks_.size()) {
                        pending_network_ = networks_[selected_index_];
                        
                        bool is_open = std::find(
                            pending_network_.security_types.begin(),
                            pending_network_.security_types.end(),
                            SecurityType::OPEN
                        ) != pending_network_.security_types.end();
                        
                        if (is_open) {
                            ConnectionConfig config;
                            config.ssid = pending_network_.ssid;
                            config.password = "";
                            config.security_type = SecurityType::OPEN;
                            config.is_enterprise = false;
                            wm_.connect(config);
                            state_ = TUIState::CONNECTING;
                        } else {
                            state_ = TUIState::PASSWORD_INPUT;
                            password_input_ = "";
                        }
                    }
                    break;
                }
                
                case 'r':
                case 'R':
                    last_scan_time_ = std::chrono::steady_clock::now();
                    scan_msg_frames_ = 75;
                    wm_.trigger_scan();
                    networks_ = wm_.scan();
                    break;
                
                case 'i':
                case 'I': {
                    if (selected_index_ < (int)networks_.size()) {
                        info_network_ = networks_[selected_index_];
                        info_password_ = "";
                        info_saved_ = false;
                        
                        auto saved = wm_.list_networks();
                        for (const auto& s : saved) {
                            std::string saved_ssid = s.ssid;
                            std::string target_ssid = info_network_.ssid;
                            
                            while (!saved_ssid.empty() && saved_ssid.back() == ' ') saved_ssid.pop_back();
                            while (!target_ssid.empty() && target_ssid.back() == ' ') target_ssid.pop_back();
                            
                            if (saved_ssid == target_ssid) {
                                info_saved_ = true;
                                info_password_ = wm_.get_password(s.id);
                                break;
                            }
                        }
                        state_ = TUIState::NETWORK_INFO;
                    }
                    break;
                }
                
                case 'f':
                case 'F': {
                    if (selected_index_ < (int)networks_.size()) {
                        const Network& net = networks_[selected_index_];
                        auto saved = wm_.list_networks();
                        for (const auto& s : saved) {
                            std::string saved_ssid = s.ssid;
                            std::string target_ssid = net.ssid;
                            
                            while (!saved_ssid.empty() && saved_ssid.back() == ' ') saved_ssid.pop_back();
                            while (!target_ssid.empty() && target_ssid.back() == ' ') target_ssid.pop_back();
                            
                            if (saved_ssid == target_ssid) {
                                if (wm_.remove_network(s.id)) {
                                    status_message_ = "[ Red Olvidada ]";
                                    status_msg_frames_ = 20;
                                    
                                    wm_.trigger_scan();
                                    networks_ = wm_.scan();
                                    
                                    selected_index_ = 0;
                                    scroll_offset_ = 0;
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
                    
                case 'q':
                case 'Q':
                    return TUIEvent::QUIT;
            }
            break;
        }
        
        case TUIState::NETWORK_INFO:
            if (ch == 'i' || ch == 'I' || ch == 'q' || ch == 'Q') {
                if (ch == 'q' || ch == 'Q') {
                    running_ = false;
                }
                state_ = TUIState::NETWORKS_LIST;
            }
            break;
        
        case TUIState::CONNECTING:
            break;

        case TUIState::PASSWORD_INPUT: {
            if (ch == 27) { // ESC
                state_ = TUIState::NETWORKS_LIST;
                password_input_.clear();
            } else if (ch == '\n' || ch == KEY_ENTER) {
                ConnectionConfig config;
                config.ssid = pending_network_.ssid;
                config.password = password_input_;
                config.security_type = pending_network_.security_types[0];
                config.is_enterprise = false;
                wm_.connect(config);
                state_ = TUIState::CONNECTING;
                password_input_.clear();
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (!password_input_.empty()) {
                    password_input_.pop_back();
                }
            } else if (ch >= 32 && ch <= 126) {
                password_input_ += static_cast<char>(ch);
            }
            break;
        }
            
        case TUIState::ERROR:
            state_ = TUIState::NETWORKS_LIST;
            break;
    }
    
    return TUIEvent::NONE;
}

void WifiTUI::handle_resize() {
    int new_rows, new_cols;
    getmaxyx(stdscr, new_rows, new_cols);
    
    if (new_rows != screen_rows_ || new_cols != screen_cols_) {
        screen_rows_ = new_rows > 0 ? new_rows : 24;
        screen_cols_ = new_cols > 0 ? new_cols : 80;
        erase();
    }
}

void WifiTUI::draw_signal_bars(int signal) {
    int bars = 0;
    if (signal >= -50) bars = 4;
    else if (signal >= -60) bars = 3;
    else if (signal >= -70) bars = 2;
    else if (signal >= -80) bars = 1;
    
    attron(COLOR_PAIR(2));
    for (int i = 0; i < 4; ++i) {
        if (i < bars) {
            addch('#');
        } else {
            addch('-');
        }
    }
    attroff(COLOR_PAIR(2));
}

void WifiTUI::draw_network_row(const Network& net, int row, bool selected) {
    if (selected) {
        attron(A_REVERSE);
    }
    
    mvprintw(row, 2, "*>");
    mvprintw(row, 6, "%s", truncate(net.ssid, 15).c_str());
    
    int bar_col = 26;
    draw_signal_bars(net.signal_strength);
    
    mvprintw(row, bar_col + 5, "%d dBm", net.signal_strength);
    
    attron(COLOR_PAIR(4));
    mvprintw(row, bar_col + 15, "[%s]", net.get_security_string().c_str());
    attroff(COLOR_PAIR(4));
    
    if (net.is_connected) {
        attron(COLOR_PAIR(1));
        mvprintw(row, screen_cols_ - 12, "[CONECTADO]");
        attroff(COLOR_PAIR(1));
    }
    
    if (selected) {
        attroff(A_REVERSE);
    }
}

std::string WifiTUI::truncate(const std::string& str, size_t width) {
    if (str.length() <= width) {
        return str;
    }
    return str.substr(0, width - 3) + "...";
}

int WifiTUI::get_max_rows() const {
    return screen_rows_ - 7;
}

int WifiTUI::get_visible_start() const {
    return scroll_offset_;
}

int WifiTUI::get_visible_end() const {
    return std::min(scroll_offset_ + get_max_rows(), (int)networks_.size());
}
