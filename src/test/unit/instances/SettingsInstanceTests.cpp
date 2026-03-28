#include "catch_amalgamated.hpp"

#include "instances/SettingsInstance.h"
#include "config/RuntimeConfigs.h"
#include "TestSupport.h"

#include <filesystem>

using namespace test_support;

TEST_CASE("SettingsInstance keeps unsaved draft isolated from RuntimeConfigs", "[instances][SettingsInstance]") {
    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::theme       = "dark";
    RuntimeConfigs::musicVolume = 0.8f;

    SettingsInstance instance;
    instance.loadFromRuntime();

    SettingsDraft d = instance.draft();
    d.setTheme("light");
    d.setMusicVolume(0.2f);
    instance.setDraft(d);

    REQUIRE(instance.hasUnsavedChanges());
    REQUIRE(RuntimeConfigs::theme == "dark");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(0.8f));

    instance.discardUnsavedChanges();
    REQUIRE_FALSE(instance.hasUnsavedChanges());
    REQUIRE(instance.draft().getTheme() == "dark");
    REQUIRE(instance.draft().getMusicVolume() == Catch::Approx(0.8f));
}

TEST_CASE("SettingsInstance saves draft to disk and clears dirty state", "[instances][SettingsInstance]") {
    TempDir temp("term4k_settings_instance");
    RuntimeConfigs::setConfigDirOverrideForTesting(temp.path().string());

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::theme       = "dark";
    RuntimeConfigs::musicVolume = 1.0f;

    SettingsInstance instance;
    instance.loadFromRuntime();

    SettingsDraft d = instance.draft();
    d.setTheme("light");
    d.setMusicVolume(0.35f);
    d.setChartOffsetMs(12);
    d.setKeyBindings({65, 83, 68, 70});
    instance.setDraft(d);

    REQUIRE(instance.hasUnsavedChanges());
    REQUIRE(instance.saveDraftForUser("alice"));
    REQUIRE_FALSE(instance.hasUnsavedChanges());

    const auto path = RuntimeConfigs::settingsFilePathForUser("alice");
    REQUIRE(std::filesystem::exists(path));

    RuntimeConfigs::theme         = "dark";
    RuntimeConfigs::musicVolume   = 0.99f;
    RuntimeConfigs::chartOffsetMs = 0;
    RuntimeConfigs::keyBindings   = {1, 2};

    REQUIRE(RuntimeConfigs::loadForUser("alice"));
    REQUIRE(RuntimeConfigs::theme == "light");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(0.35f));
    REQUIRE(RuntimeConfigs::chartOffsetMs == 12);
    REQUIRE(RuntimeConfigs::keyBindings == std::vector<uint8_t>{65, 83, 68, 70});

    RuntimeConfigs::clearConfigDirOverrideForTesting();
}


