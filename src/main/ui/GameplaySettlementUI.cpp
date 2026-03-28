#include "GameplaySettlementUI.h"

namespace ui {

GameplaySettlementUI::GameplaySettlementUI(GameplaySettlementInstance *instance) : instance_(instance) {}

void GameplaySettlementUI::bindInstance(GameplaySettlementInstance *instance) {
    instance_ = instance;
}

void GameplaySettlementUI::show() {}

void GameplaySettlementUI::hide() {}

} // namespace ui

