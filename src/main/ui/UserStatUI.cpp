#include "UserStatUI.h"

namespace ui {

UserStatUI::UserStatUI(UserStatInstance *instance) : instance_(instance) {}

void UserStatUI::bindInstance(UserStatInstance *instance) {
    instance_ = instance;
}

void UserStatUI::show() {}

void UserStatUI::hide() {}

} // namespace ui

