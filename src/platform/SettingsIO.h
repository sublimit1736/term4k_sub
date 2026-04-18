#pragma once

#include "data/SettingsDraft.h"

#include <string>

class SettingsIO {
public:
    static SettingsDraft snapshotFromRuntime();

    static void applyToRuntime(const SettingsDraft &draft);

    static bool saveDraftForUser(const SettingsDraft &draft, const std::string &username);
};
