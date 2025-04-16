
static_assert(std::is_same_v<decltype(chat_win_), std::unique_ptr<WINDOW, decltype(&delwin)>>,
              "chat_win_ must be a unique_ptr with delwin deleter");

void NCursesUI::destroy_windows() {
    // Cleanup all ncurses windows
    chat_win_.reset();
    input_win_.reset();
    settings_win_.reset();
}
