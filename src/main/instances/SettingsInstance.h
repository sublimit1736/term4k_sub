#pragma once

#include "entities/SettingsDraft.h"

#include <string>

class SettingsInstance {
public:
    void loadFromRuntime();

    const SettingsDraft &draft() const;

    void setDraft(const SettingsDraft &draft);

    bool hasUnsavedChanges() const;

    bool saveDraftForUser(const std::string &username);

    void discardUnsavedChanges();

private:
    SettingsDraft committed_;
    SettingsDraft draft_;
    bool loaded_ = false;
};

