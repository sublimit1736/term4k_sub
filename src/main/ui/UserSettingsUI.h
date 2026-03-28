#pragma once

#include "instances/SettingsInstance.h"

namespace ui {

// Placeholder UI interface for user settings screen.
class UserSettingsUI {
public:
    explicit UserSettingsUI(SettingsInstance *instance = nullptr);

    void bindInstance(SettingsInstance *instance);
    void show();
    void hide();

private:
    SettingsInstance *instance_ = nullptr;
};

} // namespace ui

