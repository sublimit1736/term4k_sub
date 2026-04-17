#include "ThemePresetService.h"

#include "config/AppDirs.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {
std::string ensureTrailingSlash(std::string path) {
    if (!path.empty() && path.back() != '/') path.push_back('/');
    return path;
}

std::string getenvOr(const char *var, const std::string &fallback) {
    const char *value = std::getenv(var);
    return (value != nullptr && value[0] != '\0') ? std::string(value) : fallback;
}

std::string trim(const std::string &value) {
    const auto left = value.find_first_not_of(" \t\r\n");
    if (left == std::string::npos) return "";
    const auto right = value.find_last_not_of(" \t\r\n");
    return value.substr(left, right - left + 1);
}

std::string devThemesDirIfPresent() {
    const std::vector<std::string> candidates = {
        ensureTrailingSlash("src/resources/themes"),
        ensureTrailingSlash("../src/resources/themes"),
    };

    std::error_code ec;
    for (const std::string &dir : candidates) {
        if (fs::exists(dir, ec) && fs::is_directory(dir, ec)) return dir;
        ec.clear();
    }
    return "";
}
} // namespace

std::string ThemePresetService::userDirOverride;
std::string ThemePresetService::systemDirOverride;

std::string ThemePresetService::userThemesDir() {
    if (!userDirOverride.empty()) return ensureTrailingSlash(userDirOverride);

    AppDirs::init();
    return ensureTrailingSlash(AppDirs::configDir() + "themes");
}

std::string ThemePresetService::systemThemesDir() {
    if (!systemDirOverride.empty()) return ensureTrailingSlash(systemDirOverride);

    const std::string home = getenvOr("HOME", ".");
    const std::string xdgData = getenvOr("XDG_DATA_HOME", home + "/.local/share");
    std::string localSystemPath = ensureTrailingSlash(xdgData + "/term4k/themes");

    std::error_code ec;
    if (fs::exists(localSystemPath, ec) && fs::is_directory(localSystemPath, ec)) return localSystemPath;

    return "/usr/share/term4k/themes/";
}

std::string ThemePresetService::normalizeThemeId(const std::string &themeId) {
    std::string normalized = trim(themeId);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    std::replace(normalized.begin(), normalized.end(), ' ', '_');

    std::string out;
    out.reserve(normalized.size());
    for (const char ch : normalized) {
        const bool isAlnum = std::isalnum(static_cast<unsigned char>(ch)) != 0;
        if (isAlnum || ch == '_' || ch == '-') out.push_back(ch);
    }
    return out;
}

std::string ThemePresetService::makeThemePath(const std::string &dir, const std::string &themeId) {
    if (dir.empty() || themeId.empty()) return "";
    return ensureTrailingSlash(dir) + themeId + ".json";
}

bool ThemePresetService::ensureDir(const std::string &dir) {
    if (dir.empty()) return false;

    std::error_code ec;
    fs::create_directories(dir, ec);
    return !ec;
}

std::vector<std::string> ThemePresetService::listThemeIds() {
    std::vector<std::string> ids;
    const bool usingOverrides = !userDirOverride.empty() || !systemDirOverride.empty();
    const std::string devDir = usingOverrides ? "" : devThemesDirIfPresent();
    const std::vector<std::string> dirs = {userThemesDir(), devDir, systemThemesDir()};

    for (const std::string &dir : dirs) {
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) continue;

        for (const fs::directory_entry &entry : fs::directory_iterator(dir, ec)) {
            if (ec || !entry.is_regular_file()) continue;

            const fs::path &path = entry.path();
            if (path.extension() != ".json") continue;

            const std::string id = normalizeThemeId(path.stem().string());
            if (!id.empty()) ids.push_back(id);
        }
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

bool ThemePresetService::loadThemeById(const std::string &themeId, JsonUtils &out) {
    const std::string normalizedId = normalizeThemeId(themeId);
    if (normalizedId.empty()) return false;

    const std::string userDir = userThemesDir();
    const bool usingOverrides = !userDirOverride.empty() || !systemDirOverride.empty();
    const std::string devDir = usingOverrides ? "" : devThemesDirIfPresent();
    const std::string systemDir = systemThemesDir();

    auto tryLoad = [&](const std::string &id) {
        const std::string userPath = makeThemePath(userDir, id);
        if (JsonUtils::loadFlatObjectFromFile(userPath, out)) return true;

        if (!devDir.empty()) {
            const std::string devPath = makeThemePath(devDir, id);
            if (JsonUtils::loadFlatObjectFromFile(devPath, out)) return true;
        }

        const std::string systemPath = makeThemePath(systemDir, id);
        return JsonUtils::loadFlatObjectFromFile(systemPath, out);
    };

    if (tryLoad(normalizedId)) return true;

    std::string alias = normalizedId;
    bool hasAlias = false;
    if (alias.find('_') != std::string::npos) {
        std::replace(alias.begin(), alias.end(), '_', '-');
        hasAlias = alias != normalizedId;
    } else if (alias.find('-') != std::string::npos) {
        std::replace(alias.begin(), alias.end(), '-', '_');
        hasAlias = alias != normalizedId;
    }

    if (hasAlias && tryLoad(alias)) return true;
    return false;
}

bool ThemePresetService::themeExists(const std::string &themeId) {
    JsonUtils loaded;
    return loadThemeById(themeId, loaded);
}

bool ThemePresetService::exportThemeToUserDir(const std::string &themeId, const JsonUtils &theme) {
    const std::string normalizedId = normalizeThemeId(themeId);
    if (normalizedId.empty()) return false;

    const std::string targetDir = userThemesDir();
    if (!ensureDir(targetDir)) return false;

    const std::string targetPath = makeThemePath(targetDir, normalizedId);
    std::ofstream out(targetPath, std::ios::trunc);
    if (!out.is_open()) return false;

    out << JsonUtils::stringifyFlatObject(theme);
    return out.good();
}

bool ThemePresetService::importThemeFileToUserDir(const std::string &sourceFilePath, std::string *outThemeId) {
    const fs::path source(sourceFilePath);
    const std::string normalizedId = normalizeThemeId(source.stem().string());
    if (normalizedId.empty()) return false;

    JsonUtils parsed;
    if (!JsonUtils::loadFlatObjectFromFile(sourceFilePath, parsed)) return false;

    if (!exportThemeToUserDir(normalizedId, parsed)) return false;

    if (outThemeId != nullptr) *outThemeId = normalizedId;
    return true;
}

void ThemePresetService::setThemeDirOverridesForTesting(const std::string &userDir, const std::string &systemDir) {
    userDirOverride = userDir;
    systemDirOverride = systemDir;
}

void ThemePresetService::clearThemeDirOverridesForTesting() {
    userDirOverride.clear();
    systemDirOverride.clear();
}
