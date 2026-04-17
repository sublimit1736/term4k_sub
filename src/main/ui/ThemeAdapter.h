#pragma once

#include <cstdint>
#include <string>

#include "utils/JsonUtils.h"

namespace ftxui {
class Color;
}

namespace ui {

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    bool operator==(const Rgb &other) const = default;
};

struct ThemePalette {
    std::string themeId;
    bool trueColor = true;

    Rgb surfaceBg;
    Rgb surfacePanel;
    Rgb borderNormal;
    Rgb textPrimary;
    Rgb textMuted;
    Rgb accentPrimary;
};

class ThemeAdapter {
public:
    // Resolves RuntimeConfigs::theme from preset files and returns a terminal-ready palette.
    static ThemePalette resolveFromRuntime();

    // Converts #RRGGBB into terminal color. In non-truecolor terminals, colors are quantized.
    static Rgb adaptHex(const std::string &hex, const Rgb &fallback, bool trueColor);

    static bool terminalSupportsTrueColor();

private:
    static ThemePalette fromThemeJson(const std::string &themeId, const JsonUtils &theme, bool trueColor);
    static bool parseHex(const std::string &hex, Rgb &out);
    static Rgb quantizeTo256(const Rgb &rgb);
};

// Shared color helpers used by UI renderers.
ftxui::Color toColor(const Rgb &rgb);
ftxui::Color highContrastOn(const Rgb &bg);

} // namespace ui
