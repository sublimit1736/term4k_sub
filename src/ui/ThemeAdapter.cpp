#include "ui/ThemeAdapter.h"

#include "utils/RuntimeConfigs.h"
#include "services/ThemePresetService.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace ui {
namespace {

Rgb kSurfaceBgFallback{30, 30, 46};
Rgb kSurfacePanelFallback{24, 24, 37};
Rgb kBorderFallback{49, 50, 68};
Rgb kTextPrimaryFallback{205, 214, 244};
Rgb kTextMutedFallback{166, 173, 200};
Rgb kAccentPrimaryFallback{137, 180, 250};

int hexToNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

uint8_t parseHexByte(char hi, char lo) {
    const int h = hexToNibble(hi);
    const int l = hexToNibble(lo);
    if (h < 0 || l < 0) return 0;
    return static_cast<uint8_t>((h << 4) | l);
}

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

uint8_t nearestCubeChannel(uint8_t c) {
    static constexpr std::array<uint8_t, 6> cube = {0, 95, 135, 175, 215, 255};

    uint8_t best = cube.front();
    int bestDist = 1 << 30;
    for (const uint8_t candidate : cube) {
        const int dist = std::abs(static_cast<int>(c) - static_cast<int>(candidate));
        if (dist < bestDist) {
            bestDist = dist;
            best = candidate;
        }
    }
    return best;
}

} // namespace

ThemePalette ThemeAdapter::resolveFromRuntime() {
    const bool trueColor = terminalSupportsTrueColor();
    const std::string themeId = RuntimeConfigs::theme;

    JsonUtils theme;
    if (!ThemePresetService::loadThemeById(themeId, theme)) {
        ThemePalette fallback;
        fallback.themeId = themeId.empty() ? "tomorrow-night" : themeId;
        fallback.trueColor = trueColor;
        fallback.surfaceBg = trueColor ? kSurfaceBgFallback : quantizeTo256(kSurfaceBgFallback);
        fallback.surfacePanel = trueColor ? kSurfacePanelFallback : quantizeTo256(kSurfacePanelFallback);
        fallback.borderNormal = trueColor ? kBorderFallback : quantizeTo256(kBorderFallback);
        fallback.textPrimary = trueColor ? kTextPrimaryFallback : quantizeTo256(kTextPrimaryFallback);
        fallback.textMuted = trueColor ? kTextMutedFallback : quantizeTo256(kTextMutedFallback);
        fallback.accentPrimary = trueColor ? kAccentPrimaryFallback : quantizeTo256(kAccentPrimaryFallback);
        return fallback;
    }

    return fromThemeJson(themeId, theme, trueColor);
}

Rgb ThemeAdapter::adaptHex(const std::string &hex, const Rgb &fallback, bool trueColor) {
    Rgb parsed;
    if (!parseHex(hex, parsed)) parsed = fallback;
    return trueColor ? parsed : quantizeTo256(parsed);
}

bool ThemeAdapter::terminalSupportsTrueColor() {
    const char *value = std::getenv("COLORTERM");
    if (!value || value[0] == '\0') return false;
    const std::string lowered = lowerCopy(value);
    return lowered.find("truecolor") != std::string::npos ||
           lowered.find("24bit") != std::string::npos;
}

ThemePalette ThemeAdapter::fromThemeJson(const std::string &themeId, const JsonUtils &theme, bool trueColor) {
    ThemePalette palette;
    palette.themeId = themeId;
    palette.trueColor = trueColor;

    palette.surfaceBg = adaptHex(theme.get("surface.bg"), kSurfaceBgFallback, trueColor);
    palette.surfacePanel = adaptHex(theme.get("surface.panel"), kSurfacePanelFallback, trueColor);
    palette.borderNormal = adaptHex(theme.get("border.normal"), kBorderFallback, trueColor);
    palette.textPrimary = adaptHex(theme.get("text.primary"), kTextPrimaryFallback, trueColor);
    palette.textMuted = adaptHex(theme.get("text.muted"), kTextMutedFallback, trueColor);
    palette.accentPrimary = adaptHex(theme.get("accent.primary"), kAccentPrimaryFallback, trueColor);

    return palette;
}

bool ThemeAdapter::parseHex(const std::string &hex, Rgb &out) {
    if (hex.size() != 7 || hex[0] != '#') return false;

    out.r = parseHexByte(hex[1], hex[2]);
    out.g = parseHexByte(hex[3], hex[4]);
    out.b = parseHexByte(hex[5], hex[6]);

    return true;
}

Rgb ThemeAdapter::quantizeTo256(const Rgb &rgb) {
    return Rgb{
        nearestCubeChannel(rgb.r),
        nearestCubeChannel(rgb.g),
        nearestCubeChannel(rgb.b),
    };
}

} // namespace ui
