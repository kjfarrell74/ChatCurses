#pragma once
#include <functional>

class SignalHandler {
public:
    static void setup(const std::function<void()>& on_exit);
    static bool check_and_clear_resize();
};
