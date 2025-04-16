
void NCursesUI::destroy_windows() {
    // Cleanup all ncurses windows
    chat_win_.reset();
    input_win_.reset();
    settings_win_.reset();
}
