#pragma once

#include "ui/UIBus.h"

#include <ftxui/component/component.hpp>

#include <functional>

namespace ftxui {
class ScreenInteractive;
}

namespace ui {

class ChartListUI {
public:
    static ftxui::Component component(ftxui::ScreenInteractive &screen,
                                      const std::function<void(UIScene)> &onRoute);
};

} // namespace ui
