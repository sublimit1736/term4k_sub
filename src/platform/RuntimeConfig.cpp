#include "RuntimeConfig.h"

#include "AppDirs.h"
#include "platform/ThemeStore.h"
#include "utils/JsonUtils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
    constexpr const char* kDefaultTheme = "tomorrow-night";
    constexpr uint32_t kSchemaVersion   = 1;

    std::string ensureTrailingSlash(std::string path) {
        if (!path.empty() && path.back() != '/') path.push_back('/');
        return path;
    }

    std::string trim(const std::string &value) {
        const auto left = value.find_first_not_of(" \t\r\n");
        if (left == std::string::npos) return "";
        const auto right = value.find_last_not_of(" \t\r\n");
        return value.substr(left, right - left + 1);
    }

    std::string toLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    std::string canonicalThemeId(std::string value) {
        value = toLower(trim(value));
        if (value == "tomorrow_night") return "tomorrow-night";
        if (value == "tokyo_night") return "tokyo-night";
        return value;
    }

    bool parseBool(const std::string &raw, const bool fallback) {
        const std::string value = toLower(trim(raw));
        if (value == "1" || value == "true" || value == "yes" || value == "on") return true;
        if (value == "0" || value == "false" || value == "no" || value == "off") return false;
        return fallback;
    }

    float parseFloat(const std::string &raw, const float fallback, const float minV, const float maxV) {
        try{
            const float parsed = std::stof(trim(raw));
            return std::clamp(parsed, minV, maxV);
        }
        catch (...){
            return fallback;
        }
    }

    uint32_t parseUint32(const std::string &raw, const uint32_t fallback, const uint32_t minV, const uint32_t maxV) {
        try{
            const unsigned long parsed = std::stoul(trim(raw));
            return static_cast<uint32_t>(std::clamp<unsigned long>(parsed, minV, maxV));
        }
        catch (...){
            return fallback;
        }
    }

    int32_t parseInt32(const std::string &raw, const int32_t fallback) {
        try{
            return static_cast<int32_t>(std::stol(trim(raw)));
        }
        catch (...){
            return fallback;
        }
    }

    std::string floatToString(const float value) {
        std::ostringstream os;
        os << value;
        return os.str();
    }

    std::string normalizeTheme(const std::string &raw, const std::string &fallback) {
        std::string value = canonicalThemeId(raw);
        if (!value.empty() && ThemeStore::themeExists(value)) return value;
        if (!value.empty()) return value;

        std::string fallbackValue = canonicalThemeId(fallback);
        if (!fallbackValue.empty() && ThemeStore::themeExists(fallbackValue)) return fallbackValue;
        if (!fallbackValue.empty()) return fallbackValue;

        const std::vector<std::string> discovered = ThemeStore::listThemeIds();
        if (!discovered.empty()) return discovered.front();


        return kDefaultTheme;
    }

    ChartEndTimingMode parseChartEndTimingMode(const std::string &raw, const ChartEndTimingMode fallback) {
        const std::string value = toLower(trim(raw));
        if (value == "after_audio_end") return ChartEndTimingMode::AfterAudioEnd;
        if (value == "after_chart_end") return ChartEndTimingMode::AfterChartEnd;
        return fallback;
    }

    std::string chartEndTimingModeToString(const ChartEndTimingMode mode) {
        return mode == ChartEndTimingMode::AfterAudioEnd ? "after_audio_end" : "after_chart_end";
    }

    ToastPosition parseToastPosition(const std::string &raw, const ToastPosition fallback) {
        const std::string value = toLower(trim(raw));
        if (value == "top_left")     return ToastPosition::TopLeft;
        if (value == "top_right")    return ToastPosition::TopRight;
        if (value == "bottom_left")  return ToastPosition::BottomLeft;
        if (value == "bottom_right") return ToastPosition::BottomRight;
        return fallback;
    }

    std::string toastPositionToString(const ToastPosition pos) {
        switch (pos) {
            case ToastPosition::TopLeft:     return "top_left";
            case ToastPosition::TopRight:    return "top_right";
            case ToastPosition::BottomLeft:  return "bottom_left";
            case ToastPosition::BottomRight: return "bottom_right";
        }
        return "top_right";
    }

    std::string sanitizeUserNameForPath(std::string username) {
        for (char &ch: username){
            if (ch == '/' || ch == '\\') ch = '_';
        }
        return username;
    }

    // Key bindings use comma-separated decimal values (for example: "68,70,74,75").
    std::vector<uint8_t> parseKeyBindings(const std::string &raw, const std::vector<uint8_t> &fallback) {
        std::vector<uint8_t> values;
        std::stringstream ss(raw);
        std::string token;

        while (std::getline(ss, token, ',')){
            const std::string t = trim(token);
            if (t.empty()) continue;

            try{
                const unsigned long n = std::stoul(t);
                if (n <= 255UL) values.push_back(static_cast<uint8_t>(n));
            }
            catch (...){
                return fallback;
            }
        }

        return values.empty() ? fallback : values;
    }

    std::string keyBindingsToString(const std::vector<uint8_t> &bindings) {
        std::ostringstream os;
        for (std::size_t i = 0; i < bindings.size(); ++i){
            if (i != 0) os << ',';
            os << static_cast<unsigned int>(bindings[i]);
        }
        return os.str();
    }
} // namespace

std::string RuntimeConfig::theme                     = kDefaultTheme;
std::string RuntimeConfig::locale                    = "en_US";
float RuntimeConfig::musicVolume                     = 1.0f;
float RuntimeConfig::hitSoundVolume                  = 1.0f;
uint32_t RuntimeConfig::audioBufferSize              = 512;
bool RuntimeConfig::showEarlyLate                    = true;
bool RuntimeConfig::showAPIndicator                  = true;
bool RuntimeConfig::showFCIndicator                  = true;
int32_t RuntimeConfig::chartOffsetMs                 = 0;
int32_t RuntimeConfig::chartDisplayOffsetMs          = 0;
uint32_t RuntimeConfig::chartPreloadMs               = 2000;
ChartEndTimingMode RuntimeConfig::chartEndTimingMode = ChartEndTimingMode::AfterChartEnd;
std::vector<uint8_t> RuntimeConfig::keyBindings      = {'D', 'F', 'H', 'J', 0, 0, 0, 0, 0, 0};
ToastPosition RuntimeConfig::toastPosition           = ToastPosition::TopRight;
bool RuntimeConfig::hudShowScore                     = true;
bool RuntimeConfig::hudShowAccuracy                  = true;
bool RuntimeConfig::hudShowCombo                     = true;
bool RuntimeConfig::hudShowMaxCombo                  = false;
bool RuntimeConfig::hudShowJudgements                = false;
bool RuntimeConfig::hudShowProgress                  = false;
bool RuntimeConfig::hudShowMaxAccCeiling             = false;
bool RuntimeConfig::hudShowPbDelta                   = false;
std::string RuntimeConfig::configDirOverride;
bool RuntimeConfig::lastLoadFailed                   = false;

void RuntimeConfig::resetToDefaults() {
    theme  = normalizeTheme(kDefaultTheme, kDefaultTheme);
    locale = "en_US";

    musicVolume     = 1.0f;
    hitSoundVolume  = 1.0f;
    audioBufferSize = 512;

    showEarlyLate        = true;
    showAPIndicator      = true;
    showFCIndicator      = true;
    chartOffsetMs        = 0;
    chartDisplayOffsetMs = 0;
    chartPreloadMs       = 2000;
    chartEndTimingMode   = ChartEndTimingMode::AfterChartEnd;
    keyBindings          = {'D', 'F', 'H', 'J', 0, 0, 0, 0, 0, 0};
    toastPosition        = ToastPosition::TopRight;
    hudShowScore         = true;
    hudShowAccuracy      = true;
    hudShowCombo         = true;
    hudShowMaxCombo      = false;
    hudShowJudgements    = false;
    hudShowProgress      = false;
    hudShowMaxAccCeiling = false;
    hudShowPbDelta       = false;
    lastLoadFailed       = false;
}

std::string RuntimeConfig::settingsFilePathForUser(const std::string &username) {
    const std::string safeUsername = sanitizeUserNameForPath(username);
    if (safeUsername.empty()) return "";

    const std::string baseDir =
        configDirOverride.empty() ? ensureTrailingSlash(AppDirs::configDir()) : ensureTrailingSlash(configDirOverride);
    return baseDir + safeUsername + "_settings.json";
}

bool RuntimeConfig::loadForUser(const std::string &username) {
    if (username.empty()) return false;

    AppDirs::init();
    resetToDefaults();

    JsonUtils json;
    const std::string path = settingsFilePathForUser(username);
    if (path.empty()) return false;
    if (!JsonUtils::loadFlatObjectFromFile(path, json)) {
        lastLoadFailed = true;
        return false;
    }

    // loadForUser resets to defaults first so missing keys naturally fall back.
    theme  = normalizeTheme(json.get("appearance.theme", theme), theme);
    locale = trim(json.get("appearance.locale", locale));
    if (locale.empty()) locale = "zh_CN";

    musicVolume    = parseFloat(json.get("audio.musicVolume", floatToString(musicVolume)), musicVolume, 0.0f, 1.0f);
    hitSoundVolume = parseFloat(json.get("audio.hitSoundVolume", floatToString(hitSoundVolume)), hitSoundVolume, 0.0f,
                                1.0f);
    audioBufferSize = parseUint32(json.get("audio.bufferSize", std::to_string(audioBufferSize)), audioBufferSize, 64,
                                  8192);

    showEarlyLate   = parseBool(json.get("gameplay.showEarlyLate", showEarlyLate ? "true" : "false"), showEarlyLate);
    showAPIndicator = parseBool(json.get("gameplay.showAPIndicator", showAPIndicator ? "true" : "false"),
                                showAPIndicator);
    showFCIndicator = parseBool(json.get("gameplay.showFCIndicator", showFCIndicator ? "true" : "false"),
                                showFCIndicator);
    chartOffsetMs        = parseInt32(json.get("gameplay.chartOffsetMs", std::to_string(chartOffsetMs)), chartOffsetMs);
    chartDisplayOffsetMs = parseInt32(json.get("gameplay.chartDisplayOffsetMs", std::to_string(chartDisplayOffsetMs)),
                                      chartDisplayOffsetMs);
    chartPreloadMs = parseUint32(json.get("gameplay.chartPreloadMs", std::to_string(chartPreloadMs)), chartPreloadMs, 0,
                                 60000);
    chartEndTimingMode = parseChartEndTimingMode(json.get("gameplay.chartEndTimingMode",
                                                          chartEndTimingModeToString(chartEndTimingMode)),
                                                 chartEndTimingMode);
    keyBindings = parseKeyBindings(json.get("gameplay.keyBindings", keyBindingsToString(keyBindings)), keyBindings);
    toastPosition = parseToastPosition(json.get("appearance.toastPosition", toastPositionToString(toastPosition)),
                                       toastPosition);

    hudShowScore         = parseBool(json.get("hud.showScore",         "true"),  true);
    hudShowAccuracy      = parseBool(json.get("hud.showAccuracy",      "true"),  true);
    hudShowCombo         = parseBool(json.get("hud.showCombo",         "true"),  true);
    hudShowMaxCombo      = parseBool(json.get("hud.showMaxCombo",      "false"), false);
    hudShowJudgements    = parseBool(json.get("hud.showJudgements",    "false"), false);
    hudShowProgress      = parseBool(json.get("hud.showProgress",      "false"), false);
    hudShowMaxAccCeiling = parseBool(json.get("hud.showMaxAccCeiling", "false"), false);
    hudShowPbDelta       = parseBool(json.get("hud.showPbDelta",       "false"), false);

    lastLoadFailed = false;
    return true;
}

bool RuntimeConfig::saveForUser(const std::string &username) {
    if (username.empty()) return false;

    AppDirs::init();

    const std::string path = settingsFilePathForUser(username);
    if (path.empty()) return false;

    std::error_code ec;
    const std::filesystem::path fsPath(path);
    if (!fsPath.parent_path().empty()){
        std::filesystem::create_directories(fsPath.parent_path(), ec);
    }

    JsonUtils json;
    json.set("schemaVersion", std::to_string(kSchemaVersion));

    // Clamp values before writing to avoid persisting invalid settings.
    json.set("appearance.theme", normalizeTheme(theme, kDefaultTheme));
    json.set("appearance.locale", locale.empty() ? "zh_CN" : locale);

    json.set("audio.musicVolume", floatToString(std::clamp(musicVolume, 0.0f, 1.0f)));
    json.set("audio.hitSoundVolume", floatToString(std::clamp(hitSoundVolume, 0.0f, 1.0f)));
    json.set("audio.bufferSize", std::to_string(std::clamp<uint32_t>(audioBufferSize, 64, 8192)));

    json.set("gameplay.showEarlyLate", showEarlyLate ? "true" : "false");
    json.set("gameplay.showAPIndicator", showAPIndicator ? "true" : "false");
    json.set("gameplay.showFCIndicator", showFCIndicator ? "true" : "false");
    json.set("gameplay.chartOffsetMs", std::to_string(chartOffsetMs));
    json.set("gameplay.chartDisplayOffsetMs", std::to_string(chartDisplayOffsetMs));
    json.set("gameplay.chartPreloadMs", std::to_string(std::clamp<uint32_t>(chartPreloadMs, 0, 60000)));
    json.set("gameplay.chartEndTimingMode", chartEndTimingModeToString(chartEndTimingMode));
    json.set("gameplay.keyBindings", keyBindings.empty() ? "68,70,72,74,0,0,0,0,0,0" : keyBindingsToString(keyBindings));
    json.set("appearance.toastPosition", toastPositionToString(toastPosition));

    json.set("hud.showScore",         hudShowScore         ? "true" : "false");
    json.set("hud.showAccuracy",      hudShowAccuracy      ? "true" : "false");
    json.set("hud.showCombo",         hudShowCombo         ? "true" : "false");
    json.set("hud.showMaxCombo",      hudShowMaxCombo      ? "true" : "false");
    json.set("hud.showJudgements",    hudShowJudgements    ? "true" : "false");
    json.set("hud.showProgress",      hudShowProgress      ? "true" : "false");
    json.set("hud.showMaxAccCeiling", hudShowMaxAccCeiling ? "true" : "false");
    json.set("hud.showPbDelta",       hudShowPbDelta       ? "true" : "false");

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;

    out << JsonUtils::stringifyFlatObject(json);
    return out.good();
}

void RuntimeConfig::setConfigDirOverrideForTesting(const std::string &dir) {
    configDirOverride = dir;
}

void RuntimeConfig::clearConfigDirOverrideForTesting() {
    configDirOverride.clear();
}
