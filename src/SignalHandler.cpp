#include "SignalHandler.hpp"
#include <csignal>
#include <atomic>
#include <unistd.h>

#include <atomic>
namespace {
    std::function<void()> g_on_exit;
    std::atomic<bool> g_resize_flag{false};
    void signal_handler(int sig) {
        if (sig == SIGWINCH) {
            g_resize_flag = true;
            return;
        }
        if (g_on_exit) g_on_exit();
        _exit(0);
    }
}

bool SignalHandler::check_and_clear_resize() {
    if (g_resize_flag) {
        g_resize_flag = false;
        return true;
    }
    return false;
}

void SignalHandler::setup(const std::function<void()>& on_exit) {
    g_on_exit = on_exit;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGWINCH, signal_handler); // Handle terminal resize
}
