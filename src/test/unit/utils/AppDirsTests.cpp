#include "catch_amalgamated.hpp"

#include "config/AppDirs.h"

TEST_CASE("AppDirs exeDir has trailing slash", "[utils][AppDirs]") {
    const std::string exeDir = AppDirs::exeDir();
    REQUIRE_FALSE(exeDir.empty());
    REQUIRE(exeDir.back() == '/');
}

TEST_CASE("AppDirs init is safe and yields non-empty directories", "[utils][AppDirs]") {
    AppDirs::init();
    const auto firstData = AppDirs::dataDir();

    AppDirs::init();

    REQUIRE_FALSE(firstData.empty());
    REQUIRE(AppDirs::dataDir() == firstData);
    REQUIRE_FALSE(AppDirs::chartsDir().empty());
    REQUIRE_FALSE(AppDirs::configDir().empty());
    REQUIRE_FALSE(AppDirs::localeDir().empty());
    REQUIRE(AppDirs::dataDir().back() == '/');
    REQUIRE(AppDirs::chartsDir().back() == '/');
    REQUIRE(AppDirs::configDir().back() == '/');
    REQUIRE(AppDirs::localeDir().back() == '/');
}


