#pragma once

#include "ui/UIBus.h"

#include <ftxui/component/component.hpp>

#include <functional>

 namespace ftxui {
class ScreenInteractive;
}

namespace ui {

class UserStatUI {
public:
    static ftxui::Component component(
        ftxui::ScreenInteractive &screen,
        std::function<void(UIScene)> onRoute
        );
};

} // namespace ui
