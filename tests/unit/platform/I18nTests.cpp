#include "catch_amalgamated.hpp"

#include "platform/I18n.h"
#include "TestSupport.h"

#include <filesystem>

using namespace test_support;

TEST_CASE (
"I18n loads flat JSON and derives locale from filename"
,
"[services][I18n]"
)
 {
    TempDir temp("term4k_i18n");
    const auto file = temp.path() / "zh_CN.json";
    writeTextFile(file, "{\"menu.start\":\"Start\",\"menu.exit\":\"Exit\"}");

    auto &i18n = I18n::instance();
    REQUIRE(i18n.load(file.string()));
    REQUIRE(i18n.locale() == "zh_CN");
    REQUIRE(i18n.get("menu.start") == "Start");
    REQUIRE(i18n("menu.exit") == "Exit");
}

TEST_CASE (
"I18n falls back to key when translation is missing"
,
"[services][I18n]"
)
 {
    auto &i18n = I18n::instance();
    REQUIRE(i18n.get("missing.key") == "missing.key");
}

TEST_CASE (
"I18n returns false when locale file does not exist"
,
"[services][I18n]"
)
 {
    auto &i18n = I18n::instance();
    REQUIRE_FALSE(i18n.load("/definitely/not/found/i18n.json"));
}

TEST_CASE(
"I18n ensureLocaleLoaded prefers development src/resources/i18n path",
"[services][I18n]"
)
{
    TempDir temp("term4k_i18n_dev_path");
    const auto oldCwd = std::filesystem::current_path();
    std::filesystem::current_path(temp.path());

    std::filesystem::create_directories(temp.path() / "src/resources/i18n");
    writeTextFile(temp.path() / "src/resources/i18n/en_US.json",
                  "{\"ui.sample\":\"dev-value\"}");

    auto &i18n = I18n::instance();
    REQUIRE(i18n.ensureLocaleLoaded("en_US"));
    REQUIRE(i18n.get("ui.sample") == "dev-value");

    std::filesystem::current_path(oldCwd);
}
