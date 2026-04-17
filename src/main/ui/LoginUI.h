#pragma once

namespace ftxui {
class ScreenInteractive;
}

namespace ui {

class LoginUI {
public:
    static int run();
    static int run(ftxui::ScreenInteractive &screen);
};

} // namespace ui
