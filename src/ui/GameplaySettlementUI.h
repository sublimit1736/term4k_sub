#pragma once

#include "ui/UIBus.h"

#include <ftxui/component/component.hpp>

#include <functional>

namespace ui {

class GameplaySettlementUI {
public:
    static ftxui::Component component(const SettlementRouteParams &params,
                                      std::function<void(UIScene)> onRoute);
};

} // namespace ui
