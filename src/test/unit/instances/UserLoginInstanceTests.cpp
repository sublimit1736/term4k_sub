#include "catch_amalgamated.hpp"

#include "instances/UserLoginInstance.h"
#include "services/AuthenticatedUserService.h"
#include "dao/UserAccountsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

using namespace test_support;

TEST_CASE("UserLoginInstance handles register/login/logout for normal users", "[instances][UserLoginInstance]") {
    TempDir temp("term4k_user_login_instance");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    UserLoginInstance instance;
    REQUIRE(instance.registerUser("user_a", "pw"));
    REQUIRE(instance.registerUser("user_b", "pw"));

    REQUIRE(instance.login("user_a", "pw"));
    REQUIRE(instance.isLoggedIn());
    REQUIRE(instance.currentUser().has_value());
    REQUIRE(instance.currentUser()->getUID() == 1000);

    instance.logout();
    REQUIRE_FALSE(instance.isLoggedIn());

    REQUIRE_FALSE(instance.login("Admin", "pw"));

    AuthenticatedUserService::logout();
    UserAccountsDAO::setDataDir(".");
}

