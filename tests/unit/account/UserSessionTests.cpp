#include "catch_amalgamated.hpp"

#include "account/UserSession.h"
#include "platform/RuntimeConfig.h"
#include "account/UserStore.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

#include <filesystem>

using namespace test_support;

TEST_CASE (

"UserService guest/admin login modes update session flags"
,
"[services][UserService]"
)
 {
    UserSession::logout();

    // Guest session should not expose UID and should not be marked as admin.
    UserSession::loginGuest();
    REQUIRE(UserSession::isGuest());
    REQUIRE_FALSE(UserSession::isAdmin());
    REQUIRE(UserSession::currentUIDString().empty());

    // Admin session should set admin flag and expose fixed UID string.
    UserSession::loginAdmin();
    REQUIRE_FALSE(UserSession::isGuest());
    REQUIRE(UserSession::isAdmin());
    REQUIRE(UserSession::currentUIDString() == "1");

    UserSession::logout();
    REQUIRE_FALSE(UserSession::currentUser().has_value());
}

TEST_CASE (

"UserService register/login uses DAO-backed credentials"
,
"[services][UserService]"
)
 {
    TempDir temp("term4k_user_service");

    UserStore::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserSession::registerUser("alice", "password", 42));
    REQUIRE_FALSE(UserSession::registerUser("alice", "password", 42));

    REQUIRE_FALSE(UserSession::login("alice", "wrong"));
    REQUIRE(UserSession::login("alice", "password"));
    REQUIRE(UserSession::currentUIDString() == "1000");

    UserSession::logout();
    UserStore::setDataDir(".");
}

TEST_CASE (

"UserService blocks reserved usernames and allocates sequential UIDs"
,
"[services][UserService]"
)
 {
    TempDir temp("term4k_user_service_admin_guard");

    UserStore::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE_FALSE(UserSession::registerUser("Admin", "admin-pass", 7));
    REQUIRE_FALSE(UserSession::login("Admin", "admin-pass"));

    REQUIRE(UserSession::registerUser("bob", "pw", 99));
    REQUIRE(UserSession::registerUser("charlie", "pw", 99));
    REQUIRE(UserSession::login("charlie", "pw"));
    REQUIRE(UserSession::currentUIDString() == "1001");

    UserSession::logout();
    UserStore::setDataDir(".");
}

TEST_CASE (

"UserService session transitions follow expected state machine"
,
"[services][UserService]"
)
 {
    TempDir temp("term4k_user_service_state_machine");
    UserStore::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserSession::registerUser("state_user", "pw", 314));

    UserSession::logout();
    REQUIRE_FALSE(UserSession::currentUser().has_value());

    // State transition: logged-out -> guest
    UserSession::loginGuest();
    REQUIRE(UserSession::isGuest());
    REQUIRE_FALSE(UserSession::isAdmin());
    REQUIRE(UserSession::currentUIDString().empty());

    // State transition: guest -> admin
    UserSession::loginAdmin();
    REQUIRE_FALSE(UserSession::isGuest());
    REQUIRE(UserSession::isAdmin());
    REQUIRE(UserSession::currentUIDString() == "1");

    // State transition: admin -> normal user
    REQUIRE(UserSession::login("state_user", "pw"));
    REQUIRE_FALSE(UserSession::isGuest());
    REQUIRE_FALSE(UserSession::isAdmin());
    REQUIRE(UserSession::currentUIDString() == "1000");

    UserSession::logout();
    REQUIRE_FALSE(UserSession::currentUser().has_value());

    UserStore::setDataDir(".");
}

TEST_CASE (

"UserService failed login does not clear existing guest or admin session"
,
"[services][UserService]"
)
 {
    TempDir temp("term4k_user_service_failed_transition");
    UserStore::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserSession::registerUser("alice2", "pw", 21));

    // Failed login should not break existing guest session.
    UserSession::loginGuest();
    REQUIRE_FALSE(UserSession::login("alice2", "wrong"));
    REQUIRE(UserSession::isGuest());
    REQUIRE_FALSE(UserSession::isAdmin());

    // Same for admin session: failed login must not pollute current state.
    UserSession::loginAdmin();
    REQUIRE_FALSE(UserSession::login("Admin", "anything"));
    REQUIRE_FALSE(UserSession::isGuest());
    REQUIRE(UserSession::isAdmin());
    REQUIRE(UserSession::currentUIDString() == "1");

    UserSession::logout();
    UserStore::setDataDir(".");
}

TEST_CASE (

"UserService register rejects illegal usernames with readable reason"
,
"[services][UserService]"
)
 {
    TempDir temp("term4k_user_service_invalid_name");

    UserStore::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    std::string error;
    REQUIRE_FALSE(UserSession::registerUser("bad/name", "pw", 0, &error));
    REQUIRE_FALSE(error.empty());

    error.clear();
    REQUIRE_FALSE(UserSession::registerUser("bad name", "pw", 0, &error));
    REQUIRE_FALSE(error.empty());

    error.clear();
    REQUIRE(UserSession::registerUser("good_name", "pw", 0, &error));
    REQUIRE(error.empty());

    UserSession::logout();
    UserStore::setDataDir(".");
}

TEST_CASE (

"UserService guest uses defaults and admin persists personalized settings"
,
"[services][UserService]"
)
 {
    TempDir dataDir("term4k_user_service_cfg_data");
    TempDir cfgDir("term4k_user_service_cfg_runtime");

    UserStore::setDataDir(dataDir.path().string());
    LiteDBUtils::setKeyFile((dataDir.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    RuntimeConfig::setConfigDirOverrideForTesting(cfgDir.path().string());
    RuntimeConfig::resetToDefaults();

    UserSession::loginGuest();
    RuntimeConfig::theme = "light";
    UserSession::logout();
    REQUIRE_FALSE(std::filesystem::exists(RuntimeConfig::settingsFilePathForUser("Guest")));

    UserSession::loginAdmin();
    RuntimeConfig::theme = "light";
    RuntimeConfig::musicVolume = 0.42f;
    UserSession::logout();
    REQUIRE(std::filesystem::exists(RuntimeConfig::settingsFilePathForUser("Admin")));

    UserSession::loginAdmin();
    REQUIRE(RuntimeConfig::theme == "light");
    REQUIRE(RuntimeConfig::musicVolume == Catch::Approx(0.42f));
    UserSession::logout();

    RuntimeConfig::clearConfigDirOverrideForTesting();
    UserStore::setDataDir(".");
}