#include "catch_amalgamated.hpp"

#include "services/I18nService.h"
#include "TestSupport.h"

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

    auto &i18n = I18nService::instance();
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
    auto &i18n = I18nService::instance();
    REQUIRE(i18n.get("missing.key") == "missing.key");
}

TEST_CASE (
"I18n returns false when locale file does not exist"
,
"[services][I18n]"
)
 {
    auto &i18n = I18nService::instance();
    REQUIRE_FALSE(i18n.load("/definitely/not/found/i18n.json"));
}