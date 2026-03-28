#pragma once

#include "instances/AdminStatInstance.h"

namespace ui {

// Placeholder UI interface for admin user statistics.
class AdminUserStatUI {
public:
    explicit AdminUserStatUI(AdminStatInstance *instance = nullptr);

    void bindInstance(AdminStatInstance *instance);
    void show();
    void hide();

private:
    AdminStatInstance *instance_ = nullptr;
};

} // namespace ui

