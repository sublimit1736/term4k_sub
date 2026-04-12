#include "catch_amalgamated.hpp"

#include "ui/UserSettingsUI.h"
#include "services/ThemePresetService.h"
#include "TestSupport.h"

#include <filesystem>
#include <fstream>

using namespace test_support;

namespace {
void writeFile(const std::string &path, const std::string &content) {
    std::ofstream out(path, std::ios::trunc);
    REQUIRE(out.is_open());
    out << content;
    REQUIRE(out.good());
}
} // namespace

TEST_CASE("UserSettingsUI theme APIs are safe without bound instance", "[ui][UserSettingsUI]") {
    ui::UserSettingsUI ui;

    REQUIRE(ui.availableThemeIds().empty());
    std::string error;
    REQUIRE_FALSE(ui.selectTheme("tomorrow-night", &error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(ui.importThemeFromFile("missing.json", nullptr, &error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(ui.exportSelectedThemeToUserDir(&error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(ui.lastError().empty());
    REQUIRE_FALSE(ui.consumeApplySucceeded());
}

TEST_CASE("UserSettingsUI delegates theme operations to SettingsInstance", "[ui][UserSettingsUI]") {
    TempDir userThemes("term4k_ui_theme_user");
    TempDir systemThemes("term4k_ui_theme_system");
    TempDir importDir("term4k_ui_theme_import");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","text.primary":"#ffffff"})");
    writeFile((importDir.path() / "my custom light.json").string(),
              R"({"id":"my_custom_light","text.primary":"#101010"})");

    SettingsInstance instance;
    instance.loadFromRuntime();

    ui::UserSettingsUI ui(&instance);

    const auto ids = ui.availableThemeIds();
    REQUIRE(ids == std::vector<std::string>{"tomorrow-night"});

    std::string error;
    REQUIRE(ui.selectTheme("tomorrow-night", &error));
    REQUIRE(error.empty());
    REQUIRE(ui.consumeApplySucceeded());
    REQUIRE_FALSE(ui.consumeApplySucceeded());

    std::string importedId;
    REQUIRE(ui.importThemeFromFile((importDir.path() / "my custom light.json").string(), &importedId, &error));
    REQUIRE(importedId == "my_custom_light");
    REQUIRE(error.empty());
    REQUIRE(ui.consumeApplySucceeded());
    REQUIRE_FALSE(ui.consumeApplySucceeded());
    REQUIRE(std::filesystem::exists(userThemes.path() / "my_custom_light.json"));

    REQUIRE(ui.exportSelectedThemeToUserDir(&error));
    REQUIRE(error.empty());

    ThemePresetService::clearThemeDirOverridesForTesting();
}


