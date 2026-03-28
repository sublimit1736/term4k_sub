#pragma once

#include "instances/GameplayInstance.h"

namespace ui {

// Placeholder UI interface for gameplay screen.
class GameplayUI {
public:
    explicit GameplayUI(GameplayInstance *instance = nullptr);

    void bindInstance(GameplayInstance *instance);
    void show();
    void hide();

private:
    GameplayInstance *instance_ = nullptr;
};

} // namespace ui

