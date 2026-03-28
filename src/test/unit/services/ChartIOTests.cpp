#include "catch_amalgamated.hpp"

#include "services/ChartIOService.h"
#include "config/RuntimeConfigs.h"
#include "TestSupport.h"

using namespace test_support;

TEST_CASE (
"ChartIO hex helpers return lowercase fixed-width strings"
,
"[services][ChartIO]"
)
 {
    REQUIRE(ChartIOService::num2hex8str(0x1Au) == "0000001a");
    REQUIRE(ChartIOService::num2hex2str(0xAFu) == "af");
}

TEST_CASE (
"ChartIO parses all note types and applies key bindings"
,
"[services][ChartIO]"
)
 {
    TempDir temp("term4k_chart");

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::chartOffsetMs = 0;
    RuntimeConfigs::keyBindings = {
        static_cast<uint8_t>('a'),
        static_cast<uint8_t>('b')
    };

    const auto chartPath = temp.path() / "demo.t4k";
    // Cover 5 note types and include one out-of-range track to verify it gets skipped.
    writeTextFile(
        chartPath,
        "t4kcb\n"
        "010000000064\n"
        "0201000000c80000012c\n"
        "033f80000000000190\n"
        "040304000001f4\n"
        "054000000000000258\n"
        "01ff00000100\n"
        "t4kce\n");

    ChartBuffer buffer;
    REQUIRE(ChartIOService::readChart(chartPath.string().c_str(), buffer, 2, 120.0f, {4, 4}));

    REQUIRE(buffer.at(100).type == click);
    REQUIRE(buffer.at(100).data == static_cast<uint32_t>('a'));

    REQUIRE(buffer.at(200).type == push);
    REQUIRE(buffer.at(200).data == static_cast<uint32_t>('b'));
    REQUIRE(buffer.at(300).type == release);
    REQUIRE(buffer.at(300).data == static_cast<uint32_t>('b'));

    REQUIRE(buffer.at(400).type == changeBPM);
    REQUIRE(buffer.at(400).data == 0x3f800000u);

    REQUIRE(buffer.at(500).type == changeTempo);
    REQUIRE(buffer.at(500).data == 0x0304u);

    REQUIRE(buffer.at(600).type == changeStreamSpeed);
    REQUIRE(buffer.at(600).data == 0x40000000u);

    REQUIRE(buffer.find(256) == buffer.end());
}

TEST_CASE (
"ChartIO rejects invalid header and clamps negative adjusted timing"
,
"[services][ChartIO]"
)
 {
    TempDir temp("term4k_chart_invalid");

    const auto badPath = temp.path() / "bad.t4k";
    writeTextFile(badPath, "bad\n010000000064\nt4kce\n");

    ChartBuffer badBuffer;
    REQUIRE_FALSE(ChartIOService::readChart(badPath.string().c_str(), badBuffer, 1, 120.0f, {4, 4}));

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::chartOffsetMs = -2500;
    RuntimeConfigs::chartPreloadMs = 2000;

    const auto clampPath = temp.path() / "clamp.t4k";
    writeTextFile(clampPath, "t4kcb\n010000000064\nt4kce\n");

    ChartBuffer clampBuffer;
    REQUIRE(ChartIOService::readChart(clampPath.string().c_str(), clampBuffer, 1, 120.0f, {4, 4}));
    REQUIRE(clampBuffer.find(0) != clampBuffer.end());

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::chartOffsetMs = 0;
    RuntimeConfigs::chartPreloadMs = 2600;

    const auto preloadPath = temp.path() / "preload.t4k";
    writeTextFile(preloadPath, "t4kcb\n010000000064\nt4kce\n");

    ChartBuffer preloadBuffer;
    REQUIRE(ChartIOService::readChart(preloadPath.string().c_str(), preloadBuffer, 1, 120.0f, {4, 4}));
    REQUIRE(preloadBuffer.find(700) != preloadBuffer.end());
}

TEST_CASE (
"ChartIO accepts absolute paths and fails cleanly on missing files"
,
"[services][ChartIO]"
)
 {
    TempDir temp("term4k_chart_absolute");
    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::chartOffsetMs = 0;

    const auto absoluteChart = temp.path() / "abs.t4k";
    writeTextFile(absoluteChart, "t4kcb\n010000000001\nt4kce\n");

    ChartBuffer absoluteBuffer;
    REQUIRE(ChartIOService::readChart(absoluteChart.string().c_str(), absoluteBuffer, 1, 120.0f, {4, 4}));
    REQUIRE(absoluteBuffer.find(1) != absoluteBuffer.end());

    const auto missingPath = temp.path() / "missing_file.t4k";
    ChartBuffer missingBuffer;
    REQUIRE_FALSE(ChartIOService::readChart(missingPath.string().c_str(), missingBuffer, 1, 120.0f, {4, 4}));
}

TEST_CASE (
"ChartIO tolerates malformed lines and only keeps valid notes"
,
"[services][ChartIO]"
)
 {
    TempDir temp("term4k_chart_malformed");
    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::chartOffsetMs = 0;
    RuntimeConfigs::keyBindings = {static_cast<uint8_t>('x')};

    // Mixed input: short line, unknown type, out-of-range track, and one valid tap.
    // Expected: only the valid event is kept.
    writeTextFile(
        temp.path() / "mixed.t4k",
        "t4kcb\n"
        "01\n"
        "ff0000000064\n"
        "010100000064\n"
        "010000000032\n"
        "t4kce\n");

    ChartBuffer buffer;
    REQUIRE(ChartIOService::readChart((temp.path() / "mixed.t4k").string().c_str(), buffer, 1, 120.0f, {4, 4}));
    REQUIRE(buffer.size() == 1);
    REQUIRE(buffer.at(50).type == click);
    REQUIRE(buffer.at(50).data == static_cast<uint32_t>('x'));
}

TEST_CASE (
"ChartIO rejects null or empty chart path"
,
"[services][ChartIO]"
)
 {
    ChartBuffer nullBuffer;
    REQUIRE_FALSE(ChartIOService::readChart(nullptr, nullBuffer, 1, 120.0f, {4, 4}));
    REQUIRE(nullBuffer.empty());

    ChartBuffer emptyBuffer;
    REQUIRE_FALSE(ChartIOService::readChart("", emptyBuffer, 1, 120.0f, {4, 4}));
    REQUIRE(emptyBuffer.empty());
}