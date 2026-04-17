#include "SettingsInstance.h"

#include "services/SettingsService.h"
#include "services/ThemePresetService.h"

void SettingsInstance::loadFromRuntime() {
    committed_ = SettingsService::snapshotFromRuntime();
    draft_     = committed_;
    loaded_    = true;
}

const SettingsDraft &SettingsInstance::draft() const {
    return draft_;
}

void SettingsInstance::setDraft(const SettingsDraft &draft) {
    draft_ = draft;
    if (!loaded_){
        committed_ = SettingsService::snapshotFromRuntime();
        loaded_    = true;
    }
}

bool SettingsInstance::hasUnsavedChanges() const {
    return loaded_ && draft_ != committed_;
}

bool SettingsInstance::saveDraftForUser(const std::string &username) {
    if (!loaded_) loadFromRuntime();
    if (!SettingsService::saveDraftForUser(draft_, username)) return false;

    committed_ = draft_;
    return true;
}

std::vector<std::string> SettingsInstance::availableThemeIds() const {
    return ThemePresetService::listThemeIds();
}

bool SettingsInstance::selectTheme(const std::string &themeId, std::string *outError) {
    if (!ThemePresetService::themeExists(themeId)) {
        if (outError != nullptr) *outError = "Theme not found: " + themeId;
        return false;
    }

    if (!loaded_){
        loadFromRuntime();
    }

    draft_.setTheme(themeId);
    if (outError != nullptr) outError->clear();
    return true;
}

bool SettingsInstance::importThemeFromFile(const std::string &sourceFilePath,
                                           std::string *outThemeId,
                                           std::string *outError) {
    std::string importedThemeId;
    if (!ThemePresetService::importThemeFileToUserDir(sourceFilePath, &importedThemeId)) {
        if (outError != nullptr) *outError = "Failed to import theme file: " + sourceFilePath;
        return false;
    }

    if (!loaded_){
        loadFromRuntime();
    }
    draft_.setTheme(importedThemeId);

    if (outThemeId != nullptr) *outThemeId = importedThemeId;
    if (outError != nullptr) outError->clear();
    return true;
}

bool SettingsInstance::exportSelectedThemeToUserDir(std::string *outError) const {
    if (!loaded_) {
        if (outError != nullptr) *outError = "Settings are not loaded.";
        return false;
    }

    JsonUtils theme;
    if (!ThemePresetService::loadThemeById(draft_.getTheme(), theme)) {
        if (outError != nullptr) *outError = "Current theme is not loadable: " + draft_.getTheme();
        return false;
    }

    if (!ThemePresetService::exportThemeToUserDir(draft_.getTheme(), theme)) {
        if (outError != nullptr) *outError = "Failed to export theme: " + draft_.getTheme();
        return false;
    }

    if (outError != nullptr) outError->clear();
    return true;
}

void SettingsInstance::discardUnsavedChanges() {
    if (!loaded_){
        loadFromRuntime();
        return;
    }
    draft_ = committed_;
}
