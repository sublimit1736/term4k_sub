#pragma once

#include "utils/JsonUtils.h"

#include <string>
#include <vector>

// Loads theme preset JSON files from user and system theme directories.
class ThemePresetService {
public:
    static std::string userThemesDir();

    static std::string systemThemesDir();

    // Returns sorted, unique theme ids from both user and system directories.
    static std::vector<std::string> listThemeIds();

    // Loads theme JSON from user dir first, then system dir.
    static bool loadThemeById(const std::string &themeId, JsonUtils &out);

    static bool themeExists(const std::string &themeId);

    // Writes a theme into user dir as <themeId>.json.
    static bool exportThemeToUserDir(const std::string &themeId, const JsonUtils &theme);

    // Imports JSON file into user dir and returns normalized id.
    static bool importThemeFileToUserDir(const std::string &sourceFilePath, std::string *outThemeId = nullptr);

    // Test-only overrides.
    static void setThemeDirOverridesForTesting(const std::string &userDir, const std::string &systemDir);

    static void clearThemeDirOverridesForTesting();

private:
    static std::string normalizeThemeId(const std::string &themeId);

    static std::string makeThemePath(const std::string &dir, const std::string &themeId);

    static bool ensureDir(const std::string &dir);

    static std::string userDirOverride;
    static std::string systemDirOverride;
};
