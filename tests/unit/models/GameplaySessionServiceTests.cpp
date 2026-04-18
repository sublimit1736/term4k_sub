#include "catch_amalgamated.hpp"

#include "services/GameplaySessionService.h"
#include "utils/RuntimeConfigs.h"
#include "services/ChartIOService.h"
#include "TestSupport.h"

using namespace test_support;

namespace {
    struct RuntimeConfigGuard {
        RuntimeConfigGuard() {
            oldChartOffsetMs  = RuntimeConfigs::chartOffsetMs;
            oldChartPreloadMs = RuntimeConfigs::chartPreloadMs;
            oldKeyBindings    = RuntimeConfigs::keyBindings;
        }

        ~RuntimeConfigGuard() {
            RuntimeConfigs::chartOffsetMs  = oldChartOffsetMs;
            RuntimeConfigs::chartPreloadMs = oldChartPreloadMs;
            RuntimeConfigs::keyBindings    = oldKeyBindings;
        }

        int32_t oldChartOffsetMs   = 0;
        uint32_t oldChartPreloadMs = 0;
        std::vector<uint8_t> oldKeyBindings;
    };

    std::string tapLine(const uint8_t lane, const uint32_t timeMs) {
        return ChartIOService::num2hex2str(tap) +
               ChartIOService::num2hex2str(lane) +
               ChartIOService::num2hex8str(timeMs);
    }

    std::string holdLine(const uint8_t lane, const uint32_t headTimeMs, const uint32_t tailTimeMs) {
        return ChartIOService::num2hex2str(hold) +
               ChartIOService::num2hex2str(lane) +
               ChartIOService::num2hex8str(headTimeMs) +
               ChartIOService::num2hex8str(tailTimeMs);
    }
} // namespace

TEST_CASE("GameplaySessionService judges tap, combo, score and auto-miss", "[services][GameplaySessionService]") {
    RuntimeConfigGuard cfg;
    RuntimeConfigs::chartOffsetMs  = 0;
    RuntimeConfigs::chartPreloadMs = 3000;
    RuntimeConfigs::keyBindings    = {65};

    TempDir temp("term4k_gameplay_tap");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 1000) + "\n" +
                  tapLine(0, 1200) + "\n" +
                  "t4kce\n");

    GameplaySessionService gameplay;
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

TEST_CASE("GameplaySessionService resolves hold by tail timing and ignores release after resolve",
          "[services][GameplaySessionService]") {
    RuntimeConfigGuard cfg;
    RuntimeConfigs::chartOffsetMs  = 0;
    RuntimeConfigs::chartPreloadMs = 3000;
    RuntimeConfigs::keyBindings    = {65};

    TempDir temp("term4k_gameplay_hold");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  holdLine(0, 1000, 1300) + "\n" +
                  "t4kce\n");

    GameplaySessionService gameplay;
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

TEST_CASE("GameplaySessionService marks hold miss on early release and head timeout", "[services][GameplaySessionService]") {
    RuntimeConfigGuard cfg;
    RuntimeConfigs::chartOffsetMs  = 0;
    RuntimeConfigs::chartPreloadMs = 3000;
    RuntimeConfigs::keyBindings    = {65};

    TempDir temp("term4k_gameplay_hold_miss");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  holdLine(0, 1000, 1300) + "\n" +
                  holdLine(0, 2000, 2200) + "\n" +
                  "t4kce\n");

    GameplaySessionService gameplay;
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

TEST_CASE("GameplaySessionService applies overlap priority and chart end settlement delay", "[services][GameplaySessionService]") {
    RuntimeConfigGuard cfg;
    RuntimeConfigs::chartOffsetMs  = 0;
    RuntimeConfigs::chartPreloadMs = 3000;
    RuntimeConfigs::keyBindings    = {65};

    TempDir temp("term4k_gameplay_overlap_settlement");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 1000) + "\n" +
                  holdLine(0, 1000, 1300) + "\n" + // tap overlaps hold head => keep tap
                  holdLine(0, 2000, 2000) + "\n" + // hold head overlaps hold tail => drop hold
                  "t4kce\n");

    GameplaySessionService gameplay;
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

TEST_CASE("GameplaySessionService exposes dual clock control for audio/chart timelines", "[services][GameplaySessionService]") {
    RuntimeConfigGuard cfg;
    RuntimeConfigs::chartOffsetMs  = 0;
    RuntimeConfigs::chartPreloadMs = 3000;
    RuntimeConfigs::keyBindings    = {65};

    TempDir temp("term4k_gameplay_clock_bridge");
    const auto chartPath = temp.path() / "chart.t4k";
    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  tapLine(0, 1000) + "\n" +
                  "t4kce\n");

    GameplaySessionService gameplay;
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


