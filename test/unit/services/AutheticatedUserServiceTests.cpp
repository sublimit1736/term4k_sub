#include "catch_amalgamated.hpp"

#include "services/AuthenticatedUserService.h"
#include "services/UserLoginService.h"
#include "dao/ProofedRecordsDAO.h"
#include "dao/UserAccountsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

using namespace test_support;

namespace {
std::string metadataJson(const std::string &id,
                         const std::string &name,
                         const float difficulty) {
    return std::string("{") +
           "\"id\":\"" + id + "\"," +
           "\"displayname\":\"" + name + "\"," +
           "\"artist\":\"artist\"," +
           "\"charter\":\"charter\"," +
           "\"BPM\":\"120\"," +
           "\"difficulty\":\"" + std::to_string(difficulty) + "\"" +
           "}";
}
}

TEST_CASE("AutheticatedUserService returns current user's verified records in reverse time order", "[services][AutheticatedUserService]") {
    TempDir temp("term4k_auth_service");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot / "chart_a");
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartsRoot / "chart_a" / "meta.json", metadataJson("chart_a", "A", 7.0f));
    writeTextFile(chartsRoot / "chart_a" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "chart_a" / "music.ogg", "dummy");

    UserAccountsDAO::setDataDir(dataRoot.string());
    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserLoginService::registerUser("alice", "pw", 5));
    REQUIRE(UserLoginService::login("alice", "pw"));
    REQUIRE(AuthenticatedUserService::syncFromUserLoginService());

    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 900000 98.00 100"));
    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 910000 99.00 300"));
    REQUIRE(ProofedRecordsDAO::addRecord("1001 chart_a song bob 880000 96.00 200"));

    const auto records = AuthenticatedUserService::loadCurrentUserVerifiedRecords(chartsRoot.string());
    REQUIRE(records.records.size() == 2);
    REQUIRE(records.order.size() == 2);
    REQUIRE(records.records.at(records.order[0]).score == 910000);
    REQUIRE(records.records.at(records.order[1]).score == 900000);

    REQUIRE_FALSE(AuthenticatedUserService::isAdminUser());
    REQUIRE_FALSE(AuthenticatedUserService::isGuestUser());

    AuthenticatedUserService::logout();
    UserLoginService::logout();
    ProofedRecordsDAO::setDataDir(".");
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("AutheticatedUserService keeps stable order for equal timestamps and falls back missing chart metadata", "[services][AutheticatedUserService]") {
    TempDir temp("term4k_auth_service_stable_fallback");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot / "chart_a");
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartsRoot / "chart_a" / "meta.json", metadataJson("chart_a", "A", 7.0f));
    writeTextFile(chartsRoot / "chart_a" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "chart_a" / "music.ogg", "dummy");

    UserAccountsDAO::setDataDir(dataRoot.string());
    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserLoginService::registerUser("alice2", "pw", 5));
    REQUIRE(UserLoginService::login("alice2", "pw"));
    REQUIRE(AuthenticatedUserService::syncFromUserLoginService());

    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice2 123456 98.00 500"));
    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_missing song alice2 654321 97.00 500"));

    const auto records = AuthenticatedUserService::loadCurrentUserVerifiedRecords(chartsRoot.string());
    REQUIRE(records.records.size() == 2);
    REQUIRE(records.order.size() == 2);

    const auto &first = records.records.at(records.order[0]);
    const auto &second = records.records.at(records.order[1]);
    REQUIRE(first.score == 123456);
    REQUIRE(second.score == 654321);
    REQUIRE(second.chart.getDisplayName() == "chart_missing");

    AuthenticatedUserService::logout();
    UserLoginService::logout();
    ProofedRecordsDAO::setDataDir(".");
    UserAccountsDAO::setDataDir(".");
}



