#include "catch_amalgamated.hpp"

#include "services/UserLoginService.h"
#include "config/RuntimeConfigs.h"
#include "dao/UserAccountsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

#include <filesystem>

using namespace test_support;

TEST_CASE("UserService guest/admin login modes update session flags", "[services][UserService]") {
    UserLoginService::logout();

    // Guest session should not expose UID and should not be marked as admin.
    UserLoginService::loginGuest();
    REQUIRE(UserLoginService::isGuest());
    REQUIRE_FALSE(UserLoginService::isAdmin());
    REQUIRE(UserLoginService::currentUIDString().empty());

    // Admin session should set admin flag and expose fixed UID string.
    UserLoginService::loginAdmin();
    REQUIRE_FALSE(UserLoginService::isGuest());
    REQUIRE(UserLoginService::isAdmin());
    REQUIRE(UserLoginService::currentUIDString() == "1");

    UserLoginService::logout();
    REQUIRE_FALSE(UserLoginService::currentUser().has_value());
}

TEST_CASE("UserService register/login uses DAO-backed credentials", "[services][UserService]") {
    TempDir temp("term4k_user_service");

    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserLoginService::registerUser("alice", "password", 42));
    REQUIRE_FALSE(UserLoginService::registerUser("alice", "password", 42));

    REQUIRE_FALSE(UserLoginService::login("alice", "wrong"));
    REQUIRE(UserLoginService::login("alice", "password"));
    REQUIRE(UserLoginService::currentUIDString() == "1000");

    UserLoginService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UserService blocks reserved usernames and allocates sequential UIDs", "[services][UserService]") {
    TempDir temp("term4k_user_service_admin_guard");

    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE_FALSE(UserLoginService::registerUser("Admin", "admin-pass", 7));
    REQUIRE_FALSE(UserLoginService::login("Admin", "admin-pass"));

    REQUIRE(UserLoginService::registerUser("bob", "pw", 99));
    REQUIRE(UserLoginService::registerUser("charlie", "pw", 99));
    REQUIRE(UserLoginService::login("charlie", "pw"));
    REQUIRE(UserLoginService::currentUIDString() == "1001");

    UserLoginService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UserService session transitions follow expected state machine", "[services][UserService]") {
    TempDir temp("term4k_user_service_state_machine");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserLoginService::registerUser("state_user", "pw", 314));

    UserLoginService::logout();
    REQUIRE_FALSE(UserLoginService::currentUser().has_value());

    // State transition: logged-out -> guest
    UserLoginService::loginGuest();
    REQUIRE(UserLoginService::isGuest());
    REQUIRE_FALSE(UserLoginService::isAdmin());
    REQUIRE(UserLoginService::currentUIDString().empty());

    // State transition: guest -> admin
    UserLoginService::loginAdmin();
    REQUIRE_FALSE(UserLoginService::isGuest());
    REQUIRE(UserLoginService::isAdmin());
    REQUIRE(UserLoginService::currentUIDString() == "1");

    // State transition: admin -> normal user
    REQUIRE(UserLoginService::login("state_user", "pw"));
    REQUIRE_FALSE(UserLoginService::isGuest());
    REQUIRE_FALSE(UserLoginService::isAdmin());
    REQUIRE(UserLoginService::currentUIDString() == "1000");

    UserLoginService::logout();
    REQUIRE_FALSE(UserLoginService::currentUser().has_value());

    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UserService failed login does not clear existing guest or admin session", "[services][UserService]") {
    TempDir temp("term4k_user_service_failed_transition");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(UserLoginService::registerUser("alice2", "pw", 21));

    // Failed login should not break existing guest session.
    UserLoginService::loginGuest();
    REQUIRE_FALSE(UserLoginService::login("alice2", "wrong"));
    REQUIRE(UserLoginService::isGuest());
    REQUIRE_FALSE(UserLoginService::isAdmin());

    // Same for admin session: failed login must not pollute current state.
    UserLoginService::loginAdmin();
    REQUIRE_FALSE(UserLoginService::login("Admin", "anything"));
    REQUIRE_FALSE(UserLoginService::isGuest());
    REQUIRE(UserLoginService::isAdmin());
    REQUIRE(UserLoginService::currentUIDString() == "1");

    UserLoginService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UserService register rejects illegal usernames with readable reason", "[services][UserService]") {
    TempDir temp("term4k_user_service_invalid_name");

    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    std::string error;
    REQUIRE_FALSE(UserLoginService::registerUser("bad/name", "pw", 0, &error));
    REQUIRE_FALSE(error.empty());

    error.clear();
    REQUIRE_FALSE(UserLoginService::registerUser("bad name", "pw", 0, &error));
    REQUIRE_FALSE(error.empty());

    error.clear();
    REQUIRE(UserLoginService::registerUser("good_name", "pw", 0, &error));
    REQUIRE(error.empty());

    UserLoginService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UserService guest uses defaults and admin persists personalized settings", "[services][UserService]") {
    TempDir dataDir("term4k_user_service_cfg_data");
    TempDir cfgDir("term4k_user_service_cfg_runtime");

    UserAccountsDAO::setDataDir(dataDir.path().string());
    LiteDBUtils::setKeyFile((dataDir.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    RuntimeConfigs::setConfigDirOverrideForTesting(cfgDir.path().string());
    RuntimeConfigs::resetToDefaults();

    UserLoginService::loginGuest();
    RuntimeConfigs::theme = "light";
    UserLoginService::logout();
    REQUIRE_FALSE(std::filesystem::exists(RuntimeConfigs::settingsFilePathForUser("Guest")));

    UserLoginService::loginAdmin();
    RuntimeConfigs::theme = "light";
    RuntimeConfigs::musicVolume = 0.42f;
    UserLoginService::logout();
    REQUIRE(std::filesystem::exists(RuntimeConfigs::settingsFilePathForUser("Admin")));

    UserLoginService::loginAdmin();
    REQUIRE(RuntimeConfigs::theme == "light");
    REQUIRE(RuntimeConfigs::musicVolume == Catch::Approx(0.42f));
    UserLoginService::logout();

    RuntimeConfigs::clearConfigDirOverrideForTesting();
    UserAccountsDAO::setDataDir(".");
}
