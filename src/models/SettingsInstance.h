#pragma once

#include "entities/SettingsDraft.h"

#include <string>
#include <vector>

class SettingsInstance {
public:
    void loadFromRuntime();

    const SettingsDraft &draft() const;

    void setDraft(const SettingsDraft &draft);

    bool hasUnsavedChanges() const;

    bool saveDraftForUser(const std::string &username);

    std::vector<std::string> availableThemeIds() const;

    bool selectTheme(const std::string &themeId, std::string *outError = nullptr);

    bool importThemeFromFile(const std::string &sourceFilePath,
                             std::string *outThemeId = nullptr,
                             std::string *outError = nullptr);

    bool exportSelectedThemeToUserDir(std::string *outError = nullptr) const;

    void discardUnsavedChanges();

private:
    SettingsDraft committed_;
    SettingsDraft draft_;
    bool loaded_ = false;
};
