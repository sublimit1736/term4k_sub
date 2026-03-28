#pragma once

#include "instances/UserStatInstance.h"

namespace ui {

// Placeholder UI interface for normal user statistics.
class UserStatUI {
public:
    explicit UserStatUI(UserStatInstance *instance = nullptr);

    void bindInstance(UserStatInstance *instance);
    void show();
    void hide();

private:
    UserStatInstance *instance_ = nullptr;
};

} // namespace ui

