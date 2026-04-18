#pragma once

#include "ui/UIBus.h"

#include <ftxui/component/component.hpp>

#include <functional>

namespace ftxui {
class ScreenInteractive;
}

namespace ui {

class SettingsUI {
public:
    static ftxui::Component component(
        std::function<void(UIScene)> onRoute
        );
};

} // namespace ui
