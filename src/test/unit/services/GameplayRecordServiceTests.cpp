#include "catch_amalgamated.hpp"

#include "services/GameplayRecordService.h"
#include "services/AuthenticatedUserService.h"
#include "services/UserLoginService.h"
#include "dao/ProofedRecordsDAO.h"
#include "dao/UserAccountsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

using namespace test_support;

TEST_CASE("GameplayRecordService saves final result for current normal user", "[services][GameplayRecordService]") {
    TempDir temp("term4k_gameplay_record_service");
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(dataRoot);

    UserAccountsDAO::setDataDir(dataRoot.string());
    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserLoginService::registerUser("player_a", "pw", 5));
    REQUIRE(UserLoginService::login("player_a", "pw"));
    REQUIRE(AuthenticatedUserService::syncFromUserLoginService());

    GameplayFinalResult result;
    result.setScore(1234);
    result.setAccuracy(98.25);
    result.setMaxCombo(77);

    REQUIRE(GameplayRecordService::saveFinalResult(result, "chart_alpha", "song_alpha", 123456));

    const auto records = ProofedRecordsDAO::readVerifiedRecordByUID("1000");
    REQUIRE(records.size() == 1);
    REQUIRE(records.front() == "1000 chart_alpha song_alpha player_a 1234 98.25 123456 77");

    AuthenticatedUserService::logout();
    UserLoginService::logout();
    ProofedRecordsDAO::setDataDir(".");
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("GameplayRecordService rejects save when no logged-in normal user", "[services][GameplayRecordService]") {
    GameplayFinalResult result;
    result.setScore(1);
    result.setAccuracy(1.0);
    result.setMaxCombo(1);

    AuthenticatedUserService::logout();
    REQUIRE_FALSE(GameplayRecordService::saveFinalResult(result, "chart_alpha", "song_alpha", 1));
}


