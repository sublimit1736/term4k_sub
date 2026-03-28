#pragma once

#include "instances/GameplaySettlementInstance.h"

namespace ui {

// Placeholder UI interface for gameplay settlement screen.
class GameplaySettlementUI {
public:
    explicit GameplaySettlementUI(GameplaySettlementInstance *instance = nullptr);

    void bindInstance(GameplaySettlementInstance *instance);
    void show();
    void hide();

private:
    GameplaySettlementInstance *instance_ = nullptr;
};

} // namespace ui

