//
// Created by 32402 on 2026/3/26.
//

#ifndef TERM4K_RUNTIMECONFIGS_H
#define TERM4K_RUNTIMECONFIGS_H


#include <cstdint>
#include <string>
#include <vector>

// Global runtime settings hub: defaults, per-user load, and persistence.
class RuntimeConfigs {
public:
    // Appearance settings
    static std::string theme;
    static std::string locale;

    // Audio settings
    static float musicVolume;
    static float hitSoundVolume;
    static uint32_t audioBufferSize;

    // Gameplay settings
    static bool showEarlyLate;
    static bool showAPIndicator;
    static bool showFCIndicator;
    static int32_t chartOffsetMs;
    static uint32_t chartPreloadMs;
    static std::vector<uint8_t> keyBindings;

    static void resetToDefaults();

    static bool loadForUser(const std::string &username);

    static bool saveForUser(const std::string &username);

    static std::string settingsFilePathForUser(const std::string &username);

    // Test-only: overrides config directory to avoid polluting real user data.
    static void setConfigDirOverrideForTesting(const std::string &dir);

    static void clearConfigDirOverrideForTesting();

private:
    static std::string configDirOverride;
};


#endif //TERM4K_RUNTIMECONFIGS_H
