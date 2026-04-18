#include "catch_amalgamated.hpp"

#include "scenes/UserStatScene.h"
#include "account/UserContext.h"
#include "account/RecordStore.h"
#include "account/UserStore.h"
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

"UserStatScene handles register/login/logout for normal users"
,
"[instances][UserStatScene]"
)
 {
    TempDir temp("term4k_user_stat_login");
    UserStore::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    UserStatScene instance;
    REQUIRE(instance.registerUser("user_a", "pw"));
    REQUIRE(instance.registerUser("user_b", "pw"));

    REQUIRE(instance.login("user_a", "pw"));
    REQUIRE(instance.isLoggedIn());
    REQUIRE(instance.currentUser().has_value());
    REQUIRE(instance.currentUser()->getUID() == 1000);

    instance.logout();
    REQUIRE_FALSE(instance.isLoggedIn());

    REQUIRE_FALSE(instance.login("Admin", "pw"));

    UserContext::logout();
    UserStore::setDataDir(".");
}

TEST_CASE (

"UserStatScene computes rating and potential from b50"
,
"[instances][UserStatScene]"
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

    UserStore::setDataDir(dataRoot.string());
    RecordStore::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    UserStatScene login;
    REQUIRE(login.registerUser("alice", "pw"));
    REQUIRE(login.login("alice", "pw"));

    REQUIRE(RecordStore::addRecord("1000 chart_a song alice 900000 50.0 100 20"));
    REQUIRE(RecordStore::addRecord("1000 chart_a song alice 910000 100.0 200 40"));

    UserStatScene stat;
    REQUIRE(stat.refresh(chartsRoot.string()));
    REQUIRE(stat.records().records.size() == 2);
    REQUIRE(stat.rating() == Catch::Approx(13.125));
    REQUIRE(stat.potential() == Catch::Approx(6.5625));

    login.logout();
    UserContext::logout();
    RecordStore::setDataDir(".");
    UserStore::setDataDir(".");
}