#include "SettingsScene.h"

#include "platform/SettingsIO.h"
#include "platform/ThemeStore.h"

void SettingsScene::loadFromRuntime() {
    committed_ = SettingsIO::snapshotFromRuntime();
    draft_     = committed_;
    loaded_    = true;
}

const SettingsDraft &SettingsScene::draft() const {
    return draft_;
}

void SettingsScene::setDraft(const SettingsDraft &draft) {
    draft_ = draft;
    if (!loaded_){
        committed_ = SettingsIO::snapshotFromRuntime();
        loaded_    = true;
    }
}

bool SettingsScene::hasUnsavedChanges() const {
    return loaded_ && draft_ != committed_;
}

bool SettingsScene::saveDraftForUser(const std::string &username) {
    if (!loaded_) loadFromRuntime();
    if (!SettingsIO::saveDraftForUser(draft_, username)) return false;

    committed_ = draft_;
    return true;
}

std::vector<std::string> SettingsScene::availableThemeIds() const {
    return ThemeStore::listThemeIds();
}

bool SettingsScene::selectTheme(const std::string &themeId, std::string *outError) {
    if (!ThemeStore::themeExists(themeId)) {
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

bool SettingsScene::importThemeFromFile(const std::string &sourceFilePath,
                                           std::string *outThemeId,
                                           std::string *outError) {
    std::string importedThemeId;
    if (!ThemeStore::importThemeFileToUserDir(sourceFilePath, &importedThemeId)) {
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

bool SettingsScene::exportSelectedThemeToUserDir(std::string *outError) const {
    if (!loaded_) {
        if (outError != nullptr) *outError = "Settings are not loaded.";
        return false;
    }

    JsonUtils theme;
    if (!ThemeStore::loadThemeById(draft_.getTheme(), theme)) {
        if (outError != nullptr) *outError = "Current theme is not loadable: " + draft_.getTheme();
        return false;
    }

    if (!ThemeStore::exportThemeToUserDir(draft_.getTheme(), theme)) {
        if (outError != nullptr) *outError = "Failed to export theme: " + draft_.getTheme();
        return false;
    }

    if (outError != nullptr) outError->clear();
    return true;
}

void SettingsScene::discardUnsavedChanges() {
    if (!loaded_){
        loadFromRuntime();
        return;
    }
    draft_ = committed_;
}
