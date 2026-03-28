#include "AdminUserStatUI.h"

namespace ui {

AdminUserStatUI::AdminUserStatUI(AdminStatInstance *instance) : instance_(instance) {}

void AdminUserStatUI::bindInstance(AdminStatInstance *instance) {
    instance_ = instance;
}

void AdminUserStatUI::show() {}

void AdminUserStatUI::hide() {}

} // namespace ui

