#include "catch_amalgamated.hpp"

#include "ui/ThemeAdapter.h"
#include "config/RuntimeConfigs.h"
#include "services/ThemePresetService.h"
#include "TestSupport.h"

#include <array>
#include <cstdlib>
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

void setColorTerm(const std::string &value) {
#if defined(_WIN32)
    _putenv_s("COLORTERM", value.c_str());
#else
    setenv("COLORTERM", value.c_str(), 1);
#endif
}

class ColorTermGuard {
public:
    ColorTermGuard() {
        const char *current = std::getenv("COLORTERM");
        if (current && current[0] != '\0') {
            hadValue_ = true;
            value_ = current;
        }
    }

    ~ColorTermGuard() {
        if (hadValue_) {
            setColorTerm(value_);
            return;
        }
#if defined(_WIN32)
        _putenv_s("COLORTERM", "");
#else
        unsetenv("COLORTERM");
#endif
    }

private:
    bool hadValue_ = false;
    std::string value_;
};

bool isCubeChannel(uint8_t value) {
    static constexpr std::array<uint8_t, 6> cube = {0, 95, 135, 175, 215, 255};
    for (const uint8_t c : cube) {
        if (value == c) return true;
    }
    return false;
}

} // namespace

TEST_CASE("ThemeAdapter reads theme from RuntimeConfigs in truecolor mode", "[ui][ThemeAdapter]") {
    ColorTermGuard colorTermGuard;
    TempDir userThemes("term4k_theme_adapter_user");
    TempDir systemThemes("term4k_theme_adapter_system");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tomorrow-night.json").string(),
              R"({"id":"tomorrow-night","surface.bg":"#102030","surface.panel":"#1a1b1c","border.normal":"#121212","text.primary":"#fafafa","text.muted":"#a0a0a0","accent.primary":"#4455ff"})");

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::theme = "tomorrow-night";
    setColorTerm("truecolor");

    const ui::ThemePalette palette = ui::ThemeAdapter::resolveFromRuntime();
    REQUIRE(palette.themeId == "tomorrow-night");
    REQUIRE(palette.trueColor);
    REQUIRE(palette.surfaceBg == ui::Rgb{16, 32, 48});
    REQUIRE(palette.accentPrimary == ui::Rgb{68, 85, 255});

    ThemePresetService::clearThemeDirOverridesForTesting();
}

TEST_CASE("ThemeAdapter quantizes colors when terminal is not truecolor", "[ui][ThemeAdapter]") {
    ColorTermGuard colorTermGuard;
    TempDir userThemes("term4k_theme_adapter_user_256");
    TempDir systemThemes("term4k_theme_adapter_system_256");

    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    writeFile((systemThemes.path() / "tokyo-night.json").string(),
              R"({"id":"tokyo-night","surface.bg":"#112233","surface.panel":"#2c3e50","border.normal":"#445566","text.primary":"#abcdef","text.muted":"#8899aa","accent.primary":"#77aadd"})");

    RuntimeConfigs::resetToDefaults();
    RuntimeConfigs::theme = "tokyo-night";
    setColorTerm("ansi");

    const ui::ThemePalette palette = ui::ThemeAdapter::resolveFromRuntime();
    REQUIRE_FALSE(palette.trueColor);
    REQUIRE(isCubeChannel(palette.surfaceBg.r));
    REQUIRE(isCubeChannel(palette.surfaceBg.g));
    REQUIRE(isCubeChannel(palette.surfaceBg.b));
    REQUIRE(isCubeChannel(palette.accentPrimary.r));
    REQUIRE(isCubeChannel(palette.accentPrimary.g));
    REQUIRE(isCubeChannel(palette.accentPrimary.b));

    ThemePresetService::clearThemeDirOverridesForTesting();
}


