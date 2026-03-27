#include "WifiTUI.h"
#include <ncurses.h>
#include <iostream>
#include <csignal>

void signal_handler(int signum) {
    endwin();
    exit(signum);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    WifiTUI tui;

    if (!tui.init()) {
        std::cerr << "Error initializing TUI" << std::endl;
        return 1;
    }

    tui.run();
    return 0;
}
