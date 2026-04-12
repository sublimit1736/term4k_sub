#include "catch_amalgamated.hpp"

#include "config/RuntimeConfigs.h"
#include "TestSupport.h"

#include <filesystem>
#include <vector>

using namespace test_support;

TEST_CASE (
"RuntimeConfigs provides expected defaults"
,
"[config][RuntimeConfigs]"
)
 {
    RuntimeConfigs::resetToDefaults();

    REQUIRE(RuntimeConfigs::theme == "tomorrow-night");
    REQUIRE(RuntimeConfigs::locale == "en_US");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(1.0f));
    REQUIRE(RuntimeConfigs::hitSoundVolume == Catch::Approx(1.0f));
    REQUIRE(RuntimeConfigs::audioBufferSize == 512);
    REQUIRE(RuntimeConfigs::showEarlyLate);
    REQUIRE(RuntimeConfigs::showAPIndicator);
    REQUIRE(RuntimeConfigs::showFCIndicator);
    REQUIRE(RuntimeConfigs::chartOffsetMs == 0);
    REQUIRE(RuntimeConfigs::chartDisplayOffsetMs == 0);
    REQUIRE(RuntimeConfigs::chartPreloadMs == 2000);
    REQUIRE(RuntimeConfigs::chartEndTimingMode == ChartEndTimingMode::AfterChartEnd);

    const std::vector<uint8_t> expected = {68, 70, 74, 75};
    REQUIRE(RuntimeConfigs::keyBindings == expected);
}

TEST_CASE (
"RuntimeConfigs saves and loads per-user settings"
,
"[config][RuntimeConfigs]"
)
 {
    TempDir temp("term4k_runtime_configs");
    RuntimeConfigs::setConfigDirOverrideForTesting(temp.path().string());

    RuntimeConfigs::theme = "light";
    RuntimeConfigs::locale = "en_US";
    RuntimeConfigs::musicVolume = 0.35f;
    RuntimeConfigs::hitSoundVolume = 0.62f;
    RuntimeConfigs::audioBufferSize = 1024;
    RuntimeConfigs::showEarlyLate = false;
    RuntimeConfigs::showAPIndicator = false;
    RuntimeConfigs::showFCIndicator = true;
    RuntimeConfigs::chartOffsetMs = -12;
    RuntimeConfigs::chartDisplayOffsetMs = 17;
    RuntimeConfigs::chartPreloadMs = 2400;
    RuntimeConfigs::chartEndTimingMode = ChartEndTimingMode::AfterAudioEnd;
    RuntimeConfigs::keyBindings = {65, 83, 68, 70, 74, 75};

    REQUIRE(RuntimeConfigs::saveForUser("alice"));
    REQUIRE(std::filesystem::exists(RuntimeConfigs::settingsFilePathForUser("alice")));

    RuntimeConfigs::theme = "dark";
    RuntimeConfigs::locale = "en_US";
    RuntimeConfigs::musicVolume = 1.0f;
    RuntimeConfigs::hitSoundVolume = 1.0f;
    RuntimeConfigs::audioBufferSize = 256;
    RuntimeConfigs::showEarlyLate = true;
    RuntimeConfigs::showAPIndicator = true;
    RuntimeConfigs::showFCIndicator = false;
    RuntimeConfigs::chartOffsetMs = 50;
    RuntimeConfigs::chartDisplayOffsetMs = -50;
    RuntimeConfigs::chartPreloadMs = 100;
    RuntimeConfigs::chartEndTimingMode = ChartEndTimingMode::AfterChartEnd;
    RuntimeConfigs::keyBindings = {1, 2, 3};

    REQUIRE(RuntimeConfigs::loadForUser("alice"));
    REQUIRE(RuntimeConfigs::theme == "light");
    REQUIRE(RuntimeConfigs::locale == "en_US");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(0.35f));
    REQUIRE(RuntimeConfigs::hitSoundVolume == Catch::Approx(0.62f));
    REQUIRE(RuntimeConfigs::audioBufferSize == 1024);
    REQUIRE_FALSE(RuntimeConfigs::showEarlyLate);
    REQUIRE_FALSE(RuntimeConfigs::showAPIndicator);
    REQUIRE(RuntimeConfigs::showFCIndicator);
    REQUIRE(RuntimeConfigs::chartOffsetMs == -12);
    REQUIRE(RuntimeConfigs::chartDisplayOffsetMs == 17);
    REQUIRE(RuntimeConfigs::chartPreloadMs == 2400);
    REQUIRE(RuntimeConfigs::chartEndTimingMode == ChartEndTimingMode::AfterAudioEnd);

    const std::vector<uint8_t> expected = {65, 83, 68, 70, 74, 75};
    REQUIRE(RuntimeConfigs::keyBindings == expected);

    RuntimeConfigs::theme = "light";
    RuntimeConfigs::musicVolume = 0.2f;
    REQUIRE_FALSE(RuntimeConfigs::loadForUser("missing_user"));
    REQUIRE(RuntimeConfigs::theme == "tomorrow-night");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(1.0f));

    REQUIRE(RuntimeConfigs::settingsFilePathForUser("a/b").find("a_b_settings.json") != std::string::npos);

    RuntimeConfigs::clearConfigDirOverrideForTesting();
}