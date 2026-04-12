#pragma once

#include "entities/SettingsDraft.h"

#include <string>

class SettingsService {
public:
    static SettingsDraft snapshotFromRuntime();

    static void applyToRuntime(const SettingsDraft &draft);

    static bool saveDraftForUser(const SettingsDraft &draft, const std::string &username);
};

