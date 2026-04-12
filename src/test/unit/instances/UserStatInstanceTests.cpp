#include "catch_amalgamated.hpp"

#include "instances/UserLoginInstance.h"
#include "instances/UserStatInstance.h"
#include "services/AuthenticatedUserService.h"
#include "dao/ProofedRecordsDAO.h"
#include "dao/UserAccountsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

using namespace test_support;

namespace {
    std::string metadataJson(const std::string &id,
                             const std::string &name,
                             const float difficulty
        ) {
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

TEST_CASE (
"UserStatInstance computes rating and potential from b50"
,
"[instances][UserStatInstance]"
)
 {
    TempDir temp("term4k_user_stat_instance");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot / "chart_a");
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartsRoot / "chart_a" / "meta.json", metadataJson("chart_a", "A", 10.0f));
    writeTextFile(chartsRoot / "chart_a" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "chart_a" / "music.ogg", "dummy");

    UserAccountsDAO::setDataDir(dataRoot.string());
    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    UserLoginInstance login;
    REQUIRE(login.registerUser("alice", "pw"));
    REQUIRE(login.login("alice", "pw"));

    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 900000 50.0 100 20"));
    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 910000 100.0 200 40"));

    UserStatInstance stat;
    REQUIRE(stat.refresh(chartsRoot.string()));
    REQUIRE(stat.records().records.size() == 2);
    REQUIRE(stat.rating() == Catch::Approx(13.125));
    REQUIRE(stat.potential() == Catch::Approx(6.5625));

    login.logout();
    AuthenticatedUserService::logout();
    ProofedRecordsDAO::setDataDir(".");
    UserAccountsDAO::setDataDir(".");
}