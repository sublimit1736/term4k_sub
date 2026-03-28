#include "GameplayUI.h"

namespace ui {

GameplayUI::GameplayUI(GameplayInstance *instance) : instance_(instance) {}

void GameplayUI::bindInstance(GameplayInstance *instance) {
    instance_ = instance;
}

void GameplayUI::show() {}

void GameplayUI::hide() {}

} // namespace ui

