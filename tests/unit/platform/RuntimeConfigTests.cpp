#include "catch_amalgamated.hpp"

#include "platform/RuntimeConfig.h"
#include "TestSupport.h"

#include <filesystem>
#include <vector>

using namespace test_support;

TEST_CASE (

"RuntimeConfig provides expected defaults"
,
"[config][RuntimeConfig]"
)
 {
    RuntimeConfig::resetToDefaults();

    REQUIRE(RuntimeConfig::theme == "tomorrow-night");
    REQUIRE(RuntimeConfig::locale == "en_US");
    REQUIRE(RuntimeConfig::musicVolume == Catch::Approx(1.0f));
    REQUIRE(RuntimeConfig::hitSoundVolume == Catch::Approx(1.0f));
    REQUIRE(RuntimeConfig::audioBufferSize == 512);
    REQUIRE(RuntimeConfig::showEarlyLate);
    REQUIRE(RuntimeConfig::showAPIndicator);
    REQUIRE(RuntimeConfig::showFCIndicator);
    REQUIRE(RuntimeConfig::chartOffsetMs == 0);
    REQUIRE(RuntimeConfig::chartDisplayOffsetMs == 0);
    REQUIRE(RuntimeConfig::chartPreloadMs == 2000);
    REQUIRE(RuntimeConfig::chartEndTimingMode == ChartEndTimingMode::AfterChartEnd);

    const std::vector<uint8_t> expected = {'D', 'F', 'H', 'J', 0, 0, 0, 0, 0, 0};
    REQUIRE(RuntimeConfig::keyBindings == expected);
}

TEST_CASE (

"RuntimeConfig saves and loads per-user settings"
,
"[config][RuntimeConfig]"
)
 {
    TempDir temp("term4k_runtime_configs");
    RuntimeConfig::setConfigDirOverrideForTesting(temp.path().string());

    RuntimeConfig::theme = "light";
    RuntimeConfig::locale = "en_US";
    RuntimeConfig::musicVolume = 0.35f;
    RuntimeConfig::hitSoundVolume = 0.62f;
    RuntimeConfig::audioBufferSize = 1024;
    RuntimeConfig::showEarlyLate = false;
    RuntimeConfig::showAPIndicator = false;
    RuntimeConfig::showFCIndicator = true;
    RuntimeConfig::chartOffsetMs = -12;
    RuntimeConfig::chartDisplayOffsetMs = 17;
    RuntimeConfig::chartPreloadMs = 2400;
    RuntimeConfig::chartEndTimingMode = ChartEndTimingMode::AfterAudioEnd;
    RuntimeConfig::keyBindings = {65, 83, 68, 70, 74, 75};

    REQUIRE(RuntimeConfig::saveForUser("alice"));
    REQUIRE(std::filesystem::exists(RuntimeConfig::settingsFilePathForUser("alice")));

    RuntimeConfig::theme = "dark";
    RuntimeConfig::locale = "en_US";
    RuntimeConfig::musicVolume = 1.0f;
    RuntimeConfig::hitSoundVolume = 1.0f;
    RuntimeConfig::audioBufferSize = 256;
    RuntimeConfig::showEarlyLate = true;
    RuntimeConfig::showAPIndicator = true;
    RuntimeConfig::showFCIndicator = false;
    RuntimeConfig::chartOffsetMs = 50;
    RuntimeConfig::chartDisplayOffsetMs = -50;
    RuntimeConfig::chartPreloadMs = 100;
    RuntimeConfig::chartEndTimingMode = ChartEndTimingMode::AfterChartEnd;
    RuntimeConfig::keyBindings = {1, 2, 3};

    REQUIRE(RuntimeConfig::loadForUser("alice"));
    REQUIRE(RuntimeConfig::theme == "light");
    REQUIRE(RuntimeConfig::locale == "en_US");
    REQUIRE(RuntimeConfig::musicVolume == Catch::Approx(0.35f));
    REQUIRE(RuntimeConfig::hitSoundVolume == Catch::Approx(0.62f));
    REQUIRE(RuntimeConfig::audioBufferSize == 1024);
    REQUIRE_FALSE(RuntimeConfig::showEarlyLate);
    REQUIRE_FALSE(RuntimeConfig::showAPIndicator);
    REQUIRE(RuntimeConfig::showFCIndicator);
    REQUIRE(RuntimeConfig::chartOffsetMs == -12);
    REQUIRE(RuntimeConfig::chartDisplayOffsetMs == 17);
    REQUIRE(RuntimeConfig::chartPreloadMs == 2400);
    REQUIRE(RuntimeConfig::chartEndTimingMode == ChartEndTimingMode::AfterAudioEnd);

    const std::vector<uint8_t> expected = {65, 83, 68, 70, 74, 75};
    REQUIRE(RuntimeConfig::keyBindings == expected);

    RuntimeConfig::theme = "light";
    RuntimeConfig::musicVolume = 0.2f;
    REQUIRE_FALSE(RuntimeConfig::loadForUser("missing_user"));
    REQUIRE(RuntimeConfig::theme == "tomorrow-night");
    REQUIRE(RuntimeConfig::musicVolume == Catch::Approx(1.0f));

    REQUIRE(RuntimeConfig::settingsFilePathForUser("a/b").find("a_b_settings.json") != std::string::npos);

    RuntimeConfig::clearConfigDirOverrideForTesting();
}