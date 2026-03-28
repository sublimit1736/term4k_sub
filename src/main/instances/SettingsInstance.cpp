#include "SettingsInstance.h"

#include "services/SettingsService.h"

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

void SettingsInstance::discardUnsavedChanges() {
    if (!loaded_){
        loadFromRuntime();
        return;
    }
    draft_ = committed_;
}

