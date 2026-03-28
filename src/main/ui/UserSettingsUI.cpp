#include "UserSettingsUI.h"

namespace ui {

UserSettingsUI::UserSettingsUI(SettingsInstance *instance) : instance_(instance) {}

void UserSettingsUI::bindInstance(SettingsInstance *instance) {
    instance_ = instance;
}

void UserSettingsUI::show() {}

void UserSettingsUI::hide() {}

} // namespace ui

