#pragma once

#include "instances/UserLoginInstance.h"

namespace ui {

// Placeholder UI interface for user login screen.
class UserLoginUI {
public:
    explicit UserLoginUI(UserLoginInstance *instance = nullptr);

    void bindInstance(UserLoginInstance *instance);
    void show();
    void hide();

private:
    UserLoginInstance *instance_ = nullptr;
};

} // namespace ui

