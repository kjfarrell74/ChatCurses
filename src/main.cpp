#include "ChatbotApp.hpp"
#include <locale> // For setlocale

int main() {
    // Set locale for correct wide character handling (e.g., wcwidth in utf8_utils)
    std::setlocale(LC_ALL, "");
    ChatbotApp app;
    app.run();
    return 0;
}
