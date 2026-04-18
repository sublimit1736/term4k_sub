#include "catch_amalgamated.hpp"

#include "scenes/SettingsScene.h"
#include "platform/RuntimeConfig.h"
#include "platform/ThemeStore.h"
#include "TestSupport.h"

#include <filesystem>
#include <fstream>

using namespace test_support;

namespace {
void writeFile(const std::string &path, const std::string &content) {
    std::ofstream out(path, std::ios::trunc);
    REQUIRE(out.is_open());
    out << content;
    REQUIRE(out.good());
}
}

TEST_CASE("SettingsScene keeps unsaved draft isolated from RuntimeConfig", "[instances][SettingsScene]") {
    RuntimeConfig::resetToDefaults();
    RuntimeConfig::theme       = "tomorrow-night";
    RuntimeConfig::musicVolume = 0.8f;

    SettingsScene instance;
    instance.loadFromRuntime();

    SettingsDraft d = instance.draft();
    d.setTheme("lazyvim_light");
    d.setMusicVolume(0.2f);
    instance.setDraft(d);

    REQUIRE(instance.hasUnsavedChanges());
    REQUIRE(RuntimeConfig::theme == "tomorrow-night");
    REQUIRE(RuntimeConfig::musicVolume == Catch::Approx(0.8f));

    instance.discardUnsavedChanges();
    REQUIRE_FALSE(instance.hasUnsavedChanges());
    REQUIRE(instance.draft().getTheme() == "tomorrow-night");
    REQUIRE(instance.draft().getMusicVolume() == Catch::Approx(0.8f));
}

TEST_CASE("SettingsScene saves draft to disk and clears dirty state", "[instances][SettingsScene]") {
    TempDir temp("term4k_settings_instance");
    TempDir userThemes("term4k_settings_instance_user_themes");
    TempDir systemThemes("term4k_settings_instance_system_themes");
    RuntimeConfig::setConfigDirOverrideForTesting(temp.path().string());
    ThemeStore::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","text.primary":"#ffffff"})");
    writeFile((systemThemes.path() / "lazyvim_light.json").string(),
              R"({"id":"lazyvim_light","text.primary":"#111111"})");

    RuntimeConfig::resetToDefaults();
    RuntimeConfig::theme       = "tomorrow-night";
    RuntimeConfig::musicVolume = 1.0f;

    SettingsScene instance;
    instance.loadFromRuntime();

    SettingsDraft d = instance.draft();
    d.setTheme("lazyvim_light");
    d.setMusicVolume(0.35f);
    d.setChartOffsetMs(12);
    d.setKeyBindings({65, 83, 68, 70});
    instance.setDraft(d);

    REQUIRE(instance.hasUnsavedChanges());
    REQUIRE(instance.saveDraftForUser("alice"));
    REQUIRE_FALSE(instance.hasUnsavedChanges());

    const auto path = RuntimeConfig::settingsFilePathForUser("alice");
    REQUIRE(std::filesystem::exists(path));

    RuntimeConfig::theme         = "tomorrow-night";
    RuntimeConfig::musicVolume   = 0.99f;
    RuntimeConfig::chartOffsetMs = 0;
    RuntimeConfig::keyBindings   = {1, 2};

    REQUIRE(RuntimeConfig::loadForUser("alice"));
    REQUIRE(RuntimeConfig::theme == "lazyvim_light");
    REQUIRE(RuntimeConfig::musicVolume == Catch::Approx(0.35f));
    REQUIRE(RuntimeConfig::chartOffsetMs == 12);
    REQUIRE(RuntimeConfig::keyBindings == std::vector<uint8_t>{65, 83, 68, 70});

    RuntimeConfig::clearConfigDirOverrideForTesting();
    ThemeStore::clearThemeDirOverridesForTesting();
}

TEST_CASE("SettingsScene supports theme preset list/select/import/export", "[instances][SettingsScene]") {
    TempDir userThemes("term4k_settings_theme_user");
    TempDir systemThemes("term4k_settings_theme_system");
    TempDir importDir("term4k_settings_theme_import");

    ThemeStore::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","text.primary":"#ffffff"})");
    writeFile((systemThemes.path() / "tokyo-night.json").string(),
              R"({"id":"tokyo-night","text.primary":"#c0caf5"})");
    writeFile((importDir.path() / "paper light.json").string(),
              R"({"id":"paper_light","text.primary":"#1f1f1f"})");

    SettingsScene instance;
    instance.loadFromRuntime();

    REQUIRE(instance.availableThemeIds() == std::vector<std::string>{"tokyo-night", "tomorrow-night"});
    REQUIRE(instance.selectTheme("tokyo-night"));
    REQUIRE(instance.draft().getTheme() == "tokyo-night");
    REQUIRE_FALSE(instance.selectTheme("missing_theme"));

    std::string importedThemeId;
    REQUIRE(instance.importThemeFromFile((importDir.path() / "paper light.json").string(), &importedThemeId));
    REQUIRE(importedThemeId == "paper_light");
    REQUIRE(instance.draft().getTheme() == "paper_light");
    REQUIRE(std::filesystem::exists(userThemes.path() / "paper_light.json"));

    REQUIRE(instance.exportSelectedThemeToUserDir());

    ThemeStore::clearThemeDirOverridesForTesting();
}


