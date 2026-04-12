#include "catch_amalgamated.hpp"

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

TEST_CASE("ThemePresetService lists merged user/system theme ids", "[services][ThemePresetService]") {
    TempDir userThemes("term4k_theme_user");
    TempDir systemThemes("term4k_theme_system");

    writeFile((userThemes.path() / "tokyo-night.json").string(),
              R"({"id":"tokyo-night","text.primary":"#ffffff"})");
    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","text.primary":"#eeeeee"})");
    writeFile((systemThemes.path() / "tokyo-night.json").string(),
              R"({"id":"tokyo-night","text.primary":"#000000"})");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    const auto ids = ThemePresetService::listThemeIds();
    REQUIRE(ids == std::vector<std::string>{"tokyo-night", "tomorrow-night"});

    ThemePresetService::clearThemeDirOverridesForTesting();
}

TEST_CASE("ThemePresetService prefers user theme over system theme", "[services][ThemePresetService]") {
    TempDir userThemes("term4k_theme_user_prefer");
    TempDir systemThemes("term4k_theme_system_prefer");

    writeFile((userThemes.path() / "paper.json").string(),
              R"({"id":"paper","text.primary":"#111111"})");
    writeFile((systemThemes.path() / "paper.json").string(),
              R"({"id":"paper","text.primary":"#999999"})");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    JsonUtils loaded;
    REQUIRE(ThemePresetService::loadThemeById("paper", loaded));
    REQUIRE(loaded.get("text.primary") == "#111111");

    ThemePresetService::clearThemeDirOverridesForTesting();
}

TEST_CASE("ThemePresetService imports and exports user theme files", "[services][ThemePresetService]") {
    TempDir userThemes("term4k_theme_user_import_export");
    TempDir systemThemes("term4k_theme_system_import_export");
    TempDir importSource("term4k_theme_import_source");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    const std::string sourcePath = (importSource.path() / "My Fancy Theme.json").string();
    writeFile(sourcePath, R"({"id":"my_fancy_theme","accent.primary":"#3366ff"})");

    std::string importedThemeId;
    REQUIRE(ThemePresetService::importThemeFileToUserDir(sourcePath, &importedThemeId));
    REQUIRE(importedThemeId == "my_fancy_theme");
    REQUIRE(std::filesystem::exists(userThemes.path() / "my_fancy_theme.json"));

    JsonUtils exported;
    exported.set("id", "flat_remix_light");
    exported.set("text.primary", "#202020");
    REQUIRE(ThemePresetService::exportThemeToUserDir("Flat Remix Light", exported));
    REQUIRE(std::filesystem::exists(userThemes.path() / "flat_remix_light.json"));

    ThemePresetService::clearThemeDirOverridesForTesting();
}

