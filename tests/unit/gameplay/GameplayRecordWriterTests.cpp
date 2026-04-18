#include "catch_amalgamated.hpp"

#include "gameplay/GameplayRecordWriter.h"
#include "account/UserContext.h"
#include "account/UserSession.h"
#include "account/RecordStore.h"
#include "account/UserStore.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

using namespace test_support;

TEST_CASE("GameplayRecordWriter saves final result for current normal user", "[services][GameplayRecordWriter]") {
    TempDir temp("term4k_gameplay_record_service");
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(dataRoot);

    UserStore::setDataDir(dataRoot.string());
    RecordStore::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserSession::registerUser("player_a", "pw", 5));
    REQUIRE(UserSession::login("player_a", "pw"));
    REQUIRE(UserContext::syncFromUserLoginService());

    GameplayFinalResult result;
    result.setScore(1234);
    result.setAccuracy(98.25);
    result.setMaxCombo(77);

    REQUIRE(GameplayRecordWriter::saveFinalResult(result, "chart_alpha", "song_alpha", 123456));

    const auto records = RecordStore::readVerifiedRecordByUID("1000");
    REQUIRE(records.size() == 1);
    REQUIRE(records.front() == "1000 chart_alpha song_alpha player_a 1234 98.25 123456 77 0 0 0 0");

    UserContext::logout();
    UserSession::logout();
    RecordStore::setDataDir(".");
    UserStore::setDataDir(".");
}

TEST_CASE("GameplayRecordWriter rejects save when no logged-in normal user", "[services][GameplayRecordWriter]") {
    GameplayFinalResult result;
    result.setScore(1);
    result.setAccuracy(1.0);
    result.setMaxCombo(1);

    UserContext::logout();
    REQUIRE_FALSE(GameplayRecordWriter::saveFinalResult(result, "chart_alpha", "song_alpha", 1));
}


