#include "catch_amalgamated.hpp"

#include "instances/SettingsInstance.h"
#include "config/RuntimeConfigs.h"
#include "services/ThemePresetService.h"
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

TEST_CASE("SettingsInstance keeps unsaved draft isolated from RuntimeConfigs", "[instances][SettingsInstance]") {
    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::theme       = "tomorrow-night";
    RuntimeConfigs::musicVolume = 0.8f;

    SettingsInstance instance;
    instance.loadFromRuntime();

    SettingsDraft d = instance.draft();
    d.setTheme("lazyvim_light");
    d.setMusicVolume(0.2f);
    instance.setDraft(d);

    REQUIRE(instance.hasUnsavedChanges());
    REQUIRE(RuntimeConfigs::theme == "tomorrow-night");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(0.8f));

    instance.discardUnsavedChanges();
    REQUIRE_FALSE(instance.hasUnsavedChanges());
    REQUIRE(instance.draft().getTheme() == "tomorrow-night");
    REQUIRE(instance.draft().getMusicVolume() == Catch::Approx(0.8f));
}

TEST_CASE("SettingsInstance saves draft to disk and clears dirty state", "[instances][SettingsInstance]") {
    TempDir temp("term4k_settings_instance");
    TempDir userThemes("term4k_settings_instance_user_themes");
    TempDir systemThemes("term4k_settings_instance_system_themes");
    RuntimeConfigs::setConfigDirOverrideForTesting(temp.path().string());
    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","text.primary":"#ffffff"})");
    writeFile((systemThemes.path() / "lazyvim_light.json").string(),
              R"({"id":"lazyvim_light","text.primary":"#111111"})");

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::theme       = "tomorrow-night";
    RuntimeConfigs::musicVolume = 1.0f;

    SettingsInstance instance;
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

    const auto path = RuntimeConfigs::settingsFilePathForUser("alice");
    REQUIRE(std::filesystem::exists(path));

    RuntimeConfigs::theme         = "tomorrow-night";
    RuntimeConfigs::musicVolume   = 0.99f;
    RuntimeConfigs::chartOffsetMs = 0;
    RuntimeConfigs::keyBindings   = {1, 2};

    REQUIRE(RuntimeConfigs::loadForUser("alice"));
    REQUIRE(RuntimeConfigs::theme == "lazyvim_light");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(0.35f));
    REQUIRE(RuntimeConfigs::chartOffsetMs == 12);
    REQUIRE(RuntimeConfigs::keyBindings == std::vector<uint8_t>{65, 83, 68, 70});

    RuntimeConfigs::clearConfigDirOverrideForTesting();
    ThemePresetService::clearThemeDirOverridesForTesting();
}

TEST_CASE("SettingsInstance supports theme preset list/select/import/export", "[instances][SettingsInstance]") {
    TempDir userThemes("term4k_settings_theme_user");
    TempDir systemThemes("term4k_settings_theme_system");
    TempDir importDir("term4k_settings_theme_import");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","text.primary":"#ffffff"})");
    writeFile((systemThemes.path() / "tokyo-night.json").string(),
              R"({"id":"tokyo-night","text.primary":"#c0caf5"})");
    writeFile((importDir.path() / "paper light.json").string(),
              R"({"id":"paper_light","text.primary":"#1f1f1f"})");

    SettingsInstance instance;
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

    ThemePresetService::clearThemeDirOverridesForTesting();
}


