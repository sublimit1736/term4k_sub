#include "catch_amalgamated.hpp"

#include "gameplay/GameplaySession.h"
#include "platform/RuntimeConfig.h"
#include "chart/ChartFileReader.h"
#include "TestSupport.h"

using namespace test_support;

namespace {
    struct RuntimeConfigGuard {
        RuntimeConfigGuard() {
            oldChartOffsetMs  = RuntimeConfig::chartOffsetMs;
            oldChartPreloadMs = RuntimeConfig::chartPreloadMs;
            oldKeyBindings    = RuntimeConfig::keyBindings;
        }

        ~RuntimeConfigGuard() {
            RuntimeConfig::chartOffsetMs  = oldChartOffsetMs;
            RuntimeConfig::chartPreloadMs = oldChartPreloadMs;
            RuntimeConfig::keyBindings    = oldKeyBindings;
        }

        int32_t oldChartOffsetMs   = 0;
        uint32_t oldChartPreloadMs = 0;
        std::vector<uint8_t> oldKeyBindings;
    };

    std::string tapLine(const uint8_t lane, const uint32_t timeMs) {
        return ChartFileReader::num2hex2str(tap) +
               ChartFileReader::num2hex2str(lane) +
               ChartFileReader::num2hex8str(timeMs);
    }

    std::string holdLine(const uint8_t lane, const uint32_t headTimeMs, const uint32_t tailTimeMs) {
        return ChartFileReader::num2hex2str(hold) +
               ChartFileReader::num2hex2str(lane) +
               ChartFileReader::num2hex8str(headTimeMs) +
               ChartFileReader::num2hex8str(tailTimeMs);
    }
} // namespace

TEST_CASE("GameplaySession judges tap, combo, score and auto-miss", "[services][GameplaySession]") {
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_gameplay_tap");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 1000) + "\n" +
                  tapLine(0, 1200) + "\n" +
                  "t4kce\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));
    REQUIRE(gameplay.chartNoteCount() == 2);
    REQUIRE(gameplay.chartMaxScore() == 7);

    gameplay.advanceChartTimeMs(910);
    gameplay.onKeyDown(65); // delta=-90 => GREAT + EARLY
    gameplay.onKeyUp(65);

    gameplay.advanceChartTimeMs(1360); // second tap auto-miss after +150ms

    const auto s = gameplay.snapshot();
    REQUIRE(s.getGreatCount() == 1);
    REQUIRE(s.getPerfectCount() == 0);
    REQUIRE(s.getMissCount() == 1);
    REQUIRE(s.getEarlyCount() == 1);
    REQUIRE(s.getLateCount() == 1);
    REQUIRE(s.getCurrentCombo() == 0);
    REQUIRE(s.getMaxCombo() == 1);
    REQUIRE(s.getScore() == 1);
    REQUIRE(s.getAccuracy() == Catch::Approx(25.0));
}

TEST_CASE("GameplaySession resolves hold by tail timing and ignores release after resolve",
          "[services][GameplaySession]") {
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_gameplay_hold");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  holdLine(0, 1000, 1300) + "\n" +
                  "t4kce\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));
    REQUIRE(gameplay.chartNoteCount() == 1);
    REQUIRE(gameplay.chartMaxScore() == 3);

    gameplay.advanceChartTimeMs(1000);
    gameplay.onKeyDown(65); // hold head PERFECT

    gameplay.advanceChartTimeMs(1300); // still holding, tail auto PERFECT
    gameplay.onKeyUp(65);              // release after resolved should not affect counters

    const auto s = gameplay.snapshot();
    REQUIRE(s.getPerfectCount() == 1);
    REQUIRE(s.getMissCount() == 0);
    REQUIRE(s.getGreatCount() == 0);
    REQUIRE(s.getCurrentCombo() == 1);
    REQUIRE(s.getMaxCombo() == 1);
    REQUIRE(s.getScore() == 2);
    REQUIRE(s.getAccuracy() == Catch::Approx(100.0));
}

TEST_CASE("GameplaySession marks hold miss on early release and head timeout", "[services][GameplaySession]") {
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_gameplay_hold_miss");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  holdLine(0, 1000, 1300) + "\n" +
                  holdLine(0, 2000, 2200) + "\n" +
                  "t4kce\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));

    gameplay.advanceChartTimeMs(1000);
    gameplay.onKeyDown(65); // first hold head PERFECT
    gameplay.advanceChartTimeMs(1140);
    gameplay.onKeyUp(65); // earlier than tail by >150ms => MISS

    gameplay.advanceChartTimeMs(2110); // second hold head timed out => MISS

    const auto s = gameplay.snapshot();
    REQUIRE(s.getPerfectCount() == 0);
    REQUIRE(s.getGreatCount() == 0);
    REQUIRE(s.getMissCount() == 2);
    REQUIRE(s.getCurrentCombo() == 0);
    REQUIRE(s.getMaxCombo() == 0);
    REQUIRE(s.getScore() == 0);
    REQUIRE(s.getAccuracy() == Catch::Approx(0.0));
}

TEST_CASE("GameplaySession applies overlap priority and chart end settlement delay", "[services][GameplaySession]") {
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_gameplay_overlap_settlement");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 1000) + "\n" +
                  holdLine(0, 1000, 1300) + "\n" + // tap overlaps hold head => keep tap
                  holdLine(0, 2000, 2000) + "\n" + // hold head overlaps hold tail => drop hold
                  "t4kce\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));
    REQUIRE(gameplay.chartNoteCount() == 1);
    REQUIRE(gameplay.chartMaxScore() == 3);

    gameplay.advanceChartTimeMs(1000);
    gameplay.onKeyDown(65);
    gameplay.onKeyUp(65);

    auto s = gameplay.snapshot();
    REQUIRE(s.getPerfectCount() == 1);
    REQUIRE_FALSE(s.isSettlementAnimationTriggered());
    REQUIRE_FALSE(s.isResultReady());

    gameplay.advanceChartTimeMs(3999);
    s = gameplay.snapshot();
    REQUIRE_FALSE(s.isSettlementAnimationTriggered());
    REQUIRE_FALSE(s.isResultReady());

    gameplay.advanceChartTimeMs(4000);
    s = gameplay.snapshot();
    REQUIRE(s.isSettlementAnimationTriggered());
    REQUIRE(s.isResultReady());

    const auto f = gameplay.finalResult();
    REQUIRE(f.getPerfectCount() == 1);
    REQUIRE(f.getMissCount() == 0);
    REQUIRE(f.getChartNoteCount() == 1);
    REQUIRE(f.getChartMaxScore() == 3);
}

TEST_CASE("GameplaySession exposes dual clock control for audio/chart timelines", "[services][GameplaySession]") {
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_gameplay_clock_bridge");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 1000) + "\n" +
                  "t4kce\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));

    gameplay.advanceAudioTimeMs(400);
    auto clock = gameplay.clockSnapshot();
    REQUIRE(clock.getAudioTimeMs() == 400);
    REQUIRE(clock.getChartTimeMs() == 0);
    REQUIRE_FALSE(clock.isChartClockDrivenByAudio());

    gameplay.setChartClockDrivenByAudio(true);
    gameplay.advanceAudioTimeMs(980);
    clock = gameplay.clockSnapshot();
    REQUIRE(clock.getAudioTimeMs() == 980);
    REQUIRE(clock.getChartTimeMs() == 980);
    REQUIRE(clock.isChartClockDrivenByAudio());

    gameplay.onKeyDown(65); // delta=-20 => PERFECT
    gameplay.onKeyUp(65);

    const auto s = gameplay.snapshot();
    REQUIRE(s.getPerfectCount() == 1);
    REQUIRE(s.getScore() == 2);

    gameplay.setAudioFinished(true);
    clock = gameplay.clockSnapshot();
    REQUIRE(clock.isAudioFinished());
}

TEST_CASE("GameplaySession loads adjacent holds sharing the same (lane, time) boundary",
          "[services][GameplaySession]") {
    // Regression test: two hold notes on the same lane where note A's tail time equals
    // note B's head time must BOTH be loaded (previously the parser dropped both via
    // DropBoth conflict resolution, silently losing notes from charts that use this pattern).
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_adjacent_holds");
    const auto chartPath = temp.path() / "chart.t4k";

    // Hold A: lane=0, head=1000ms, tail=2000ms
    // Hold B: lane=0, head=2000ms, tail=3000ms  (head == A's tail → shared boundary)
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  holdLine(0, 1000, 2000) + "\n" +
                  holdLine(0, 2000, 3000) + "\n" +
                  "t4kce\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));
    // Both hold notes must be present; note count = 2
    REQUIRE(gameplay.chartNoteCount() == 2);
}

TEST_CASE("GameplaySession parses chart with CRLF line endings",
          "[services][GameplaySession]") {
    // Regression test: chart files authored on Windows have CRLF (\r\n) line endings.
    // The parser must strip the trailing \r so that begin/end markers are recognised.
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65, 83};

    TempDir temp("term4k_crlf_chart");
    const auto chartPath = temp.path() / "chart.t4k";
    // Write file with Windows-style CRLF endings.
    writeTextFile(chartPath,
                  std::string("t4kcb\r\n") +
                  tapLine(0, 1000) + "\r\n" +
                  tapLine(1, 2000) + "\r\n" +
                  "t4kce\r\n");

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 2));
    REQUIRE(gameplay.chartNoteCount() == 2);
}

TEST_CASE("GameplaySession auto-detects keyCount when passed as 0",
          "[services][GameplaySession]") {
    // Regression test: when meta.json does not specify keyCount the value is 0.
    // The session must auto-detect the required key count from the chart file itself.
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65, 83, 68, 70, 74};

    TempDir temp("term4k_autodetect_keycount");
    const auto chartPath = temp.path() / "chart.t4k";
    // Chart uses lanes 0-4 (requires keyCount >= 5).
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 500) + "\n" +
                  tapLine(4, 1000) + "\n" +
                  "t4kce\n");

    GameplaySession gameplay;
    // keyCount=0 → auto-detect → should succeed with keyCount=5
    REQUIRE(gameplay.openChart(chartPath.string(), 0));
    REQUIRE(gameplay.chartNoteCount() == 2);
}

