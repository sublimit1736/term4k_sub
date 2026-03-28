#include "UserLoginUI.h"

namespace ui {

UserLoginUI::UserLoginUI(UserLoginInstance *instance) : instance_(instance) {}

void UserLoginUI::bindInstance(UserLoginInstance *instance) {
    instance_ = instance;
}

void UserLoginUI::show() {}

void UserLoginUI::hide() {}

} // namespace ui

