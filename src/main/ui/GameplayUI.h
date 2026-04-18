#pragma once

#include "ui/UIBus.h"

#include <ftxui/component/component.hpp>

#include <functional>

namespace ftxui {
class ScreenInteractive;
}

namespace ui {

class GameplayUI {
public:
    static ftxui::Component component(ftxui::ScreenInteractive &screen,
                                      const GameplayRouteParams &params,
                                      std::function<void(UIScene)> onRoute);
};

} // namespace ui
