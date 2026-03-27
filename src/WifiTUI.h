#ifndef WIFI_TUI_H
#define WIFI_TUI_H

#include "Network.h"
#include "WifiManager.h"
#include <string>
#include <vector>
#include <functional>
#include <chrono>

enum class TUIState {
    NETWORKS_LIST,
    PASSWORD_INPUT,
    CONNECTING,
    NETWORK_INFO,
    ERROR
};

enum class TUIEvent {
    NONE,
    QUIT
};

struct WifiTUIConfig {
    bool auto_scan;
    int scan_interval_ms;
    bool show_hidden;
    bool color_output;
};

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
    void render();
    void render_header();
    void render_networks();
    void render_status_bar();
    void render_password_input();
    void render_connecting();
    void render_error();
    void render_network_info();

    TUIEvent handle_input(int ch);
    void handle_resize();

    void draw_signal_bars(int signal);
    void draw_network_row(const Network& net, int row, bool selected);
    std::string truncate(const std::string& str, size_t width);

    int get_max_rows() const;
    int get_visible_start() const;
    int get_visible_end() const;

    WifiManager& wm_;
    WifiTUIConfig config_;

    TUIState state_;
    TUIState prev_state_;
    std::string error_message_;
    std::string password_input_;
    int selected_index_;
    int scroll_offset_;
    int screen_rows_;
    int screen_cols_;
    bool running_;
    
    Network pending_network_;
    Network info_network_;
    std::string info_password_;
    bool info_saved_;
    
    std::vector<Network> networks_;
    
    int scan_msg_frames_;
    std::chrono::steady_clock::time_point last_scan_time_;
    
    std::string status_message_;
    int status_msg_frames_;
};

#endif
