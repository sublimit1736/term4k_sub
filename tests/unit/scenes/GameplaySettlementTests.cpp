#include "catch_amalgamated.hpp"

#include "gameplay/GameplaySession.h"
#include "scenes/GameplaySettlement.h"
#include "platform/RuntimeConfig.h"
#include "account/UserContext.h"
#include "account/UserSession.h"
#include "account/RecordStore.h"
#include "account/UserStore.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

#include <sstream>

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
}

TEST_CASE("GameplaySettlement captures final result and writes one record",
          "[instances][GameplaySettlement]") {
    RuntimeConfigGuard cfg;
    RuntimeConfig::chartOffsetMs  = 0;
    RuntimeConfig::chartPreloadMs = 3000;
    RuntimeConfig::keyBindings    = {65};

    TempDir temp("term4k_gameplay_settlement_instance");
    const auto chartPath = temp.path() / "chart.t4k";
    const auto dataRoot  = temp.path() / "data";
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  "0100000003e8\n" +
                  "t4kce\n");

    UserStore::setDataDir(dataRoot.string());
    RecordStore::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserSession::registerUser("settle_user", "pw", 5));
    REQUIRE(UserSession::login("settle_user", "pw"));
    REQUIRE(UserContext::syncFromUserLoginService());

    GameplaySession gameplay;
    REQUIRE(gameplay.openChart(chartPath.string(), 1));

    gameplay.advanceChartTimeMs(1000);
    gameplay.onKeyDown(65);
    gameplay.onKeyUp(65);

    const auto expected = gameplay.finalResult();

    GameplaySettlement settlement;
    REQUIRE(settlement.onEnterSettlement(gameplay, "chart_settle", "song_settle", 999));
    REQUIRE(settlement.hasFinalResult());
    REQUIRE(settlement.recordSaveSucceeded());

    // Entering settlement can only trigger saving once.
    REQUIRE_FALSE(settlement.onEnterSettlement(gameplay, "chart_settle", "song_settle", 1000));

    const auto final = settlement.finalResult();
    REQUIRE(final.getPerfectCount() == expected.getPerfectCount());
    REQUIRE(final.getGreatCount() == expected.getGreatCount());
    REQUIRE(final.getMissCount() == expected.getMissCount());
    REQUIRE(final.getScore() == expected.getScore());
    REQUIRE(final.getMaxCombo() == expected.getMaxCombo());

    const auto records = RecordStore::readVerifiedRecordByUID("1000");
    REQUIRE(records.size() == 1);

    std::istringstream iss(records.front());
    std::vector<std::string> fields;
    std::string token;
    while (iss >> token) fields.push_back(token);
    REQUIRE(fields.size() == 12);
    REQUIRE(fields[0] == "1000");
    REQUIRE(fields[1] == "chart_settle");
    REQUIRE(fields[2] == "song_settle");
    REQUIRE(fields[3] == "settle_user");
    REQUIRE(fields[4] == std::to_string(static_cast<uint32_t>(expected.getScore())));
    REQUIRE(fields[6] == "999");
    REQUIRE(fields[7] == std::to_string(expected.getMaxCombo()));
    REQUIRE(fields[8] == std::to_string(expected.getChartNoteCount()));
    REQUIRE(fields[9] == std::to_string(expected.getPerfectCount()));
    REQUIRE(fields[10] == std::to_string(expected.getEarlyCount()));
    REQUIRE(fields[11] == std::to_string(expected.getLateCount()));

    UserContext::logout();
    UserSession::logout();
    RecordStore::setDataDir(".");
    UserStore::setDataDir(".");
}




