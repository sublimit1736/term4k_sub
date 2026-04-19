#include "ui/GameplaySettlementUI.h"

#include "platform/RuntimeConfig.h"
#include "platform/I18n.h"
#include "ui/MessageOverlay.h"
#include "ui/ThemeAdapter.h"
#include "ui/UIColors.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace ui {
namespace {

// ── Formatting helpers ────────────────────────────────────────────────────────
std::string fmtScore(const uint64_t score) {
    std::string s = std::to_string(score);
    int insertPos = static_cast<int>(s.size()) - 3;
    while (insertPos > 0) {
        s.insert(static_cast<std::size_t>(insertPos), ",");
        insertPos -= 3;
    }
    return s;
}

std::string fmtAcc(const double acc) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << (acc * 100.0) << "%";
    return out.str();
}

// ── Grade / rank helpers ──────────────────────────────────────────────────────
struct GradeInfo {
    std::string label;
    ftxui::Color color = ftxui::Color::Default;
    bool rainbow       = false;
};

GradeInfo gradeFromAcc(const double acc) {
    const double pct = acc * 100.0;
    if (pct >= 100.0)  return {"SSS+", ftxui::Color::White,           true};
    if (pct >= 99.5)   return {"SSS",  ftxui::Color::RGB(255,215,0),  false};
    if (pct >= 99.0)   return {"SS+",  ftxui::Color::RGB(192,192,192),false};
    if (pct >= 98.0)   return {"SS",   ftxui::Color::RGB(255, 80, 40),false};
    if (pct >= 97.0)   return {"S+",   ftxui::Color::RGB(150,  0,  0),false};
    if (pct >= 95.0)   return {"S",    ftxui::Color::RGB(170, 85,255),false};
    if (pct >= 90.0)   return {"A",    ftxui::Color::RGB(255,105,180),false};
    if (pct >= 80.0)   return {"B",    ftxui::Color::RGB(255,240,160),false};
    if (pct >= 70.0)   return {"C",    ftxui::Color::RGB(120,200,255),false};
    if (pct >= 60.0)   return {"D",    ftxui::Color::RGB(120,255,120),false};
    return {"-",  ftxui::Color::RGB(130,130,130),false};
}

// ── Block-art renderer (shared with ChartListUI pattern) ─────────────────────
const std::array<ftxui::Color, 7> kRainbow = {
    ftxui::Color::RGB(255,   0,   0),
    ftxui::Color::RGB(255, 127,   0),
    ftxui::Color::RGB(255, 255,   0),
    ftxui::Color::RGB(  0, 200,   0),
    ftxui::Color::RGB(  0, 120, 255),
    ftxui::Color::RGB( 75,   0, 130),
    ftxui::Color::RGB(148,   0, 211),
};

const std::map<char, std::array<std::string, 6>> kBlockGlyph = {
    {'A', {" █████╗ ", "██╔══██╗", "███████║", "██╔══██║", "██║  ██║", "╚═╝  ╚═╝"}},
    {'B', {"██████╗ ", "██╔══██╗", "██████╔╝", "██╔══██╗", "██████╔╝", "╚═════╝ "}},
    {'C', {" ██████╗", "██╔════╝", "██║     ", "██║     ", "╚██████╗", " ╚═════╝"}},
    {'D', {"██████╗ ", "██╔══██╗", "██║  ██║", "██║  ██║", "██████╔╝", "╚═════╝ "}},
    {'F', {"███████╗", "██╔════╝", "█████╗  ", "██╔══╝  ", "██║     ", "╚═╝     "}},
    {'L', {"██╗     ", "██║     ", "██║     ", "██║     ", "███████╗", "╚══════╝"}},
    {'P', {"██████╗ ", "██╔══██╗", "██████╔╝", "██╔═══╝ ", "██║     ", "╚═╝     "}},
    {'S', {"███████╗", "██╔════╝", "███████╗", "╚════██║", "███████║", "╚══════╝"}},
    {'T', {"████████╗", "╚══██╔══╝", "   ██║   ", "   ██║   ", "   ██║   ", "   ╚═╝   "}},
    {'U', {"██╗   ██╗", "██║   ██║", "██║   ██║", "██║   ██║", "╚██████╔╝", " ╚═════╝ "}},
    {'+', {"  ██╗  ", "██████╗", "╚═██╔═╝", "  ╚═╝  ", "       ", "       "}},
};

ftxui::Element colorToken(const std::string &token, const ftxui::Color &baseColor, const bool rainbow) {
    using namespace ftxui;
    if (!rainbow) return text(token) | bold | color(baseColor);
    Elements chars;
    std::size_t ci = 0;
    for (char ch : token) {
        if (ch == ' ') { chars.push_back(text(" ")); continue; }
        chars.push_back(text(std::string(1, ch)) | bold |
                        color(kRainbow[ci % kRainbow.size()]));
        ++ci;
    }
    return hbox(std::move(chars));
}

ftxui::Element blockArtWord(const std::string &word, const ftxui::Color &baseColor, const bool rainbow) {
    using namespace ftxui;
    std::string core = word;
    bool superPlus   = false;
    if (!core.empty() && core.back() == '+') { superPlus = true; core.pop_back(); }

    std::array<std::string, 6> lines = {"", "", "", "", "", ""};
    for (char ch : core) {
        const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        const auto it = kBlockGlyph.find(up);
        if (it == kBlockGlyph.end()) continue;
        for (std::size_t i = 0; i < lines.size(); ++i)
            lines[i] += it->second[i] + " ";
    }
    if (superPlus) {
        const auto pit = kBlockGlyph.find('+');
        if (pit != kBlockGlyph.end()) {
            for (std::size_t i = 0; i < 4; ++i)
                lines[i] += pit->second[i];
        }
    }
    return vbox({
        colorToken(lines[0], baseColor, rainbow),
        colorToken(lines[1], baseColor, rainbow),
        colorToken(lines[2], baseColor, rainbow),
        colorToken(lines[3], baseColor, rainbow),
        colorToken(lines[4], baseColor, rainbow),
        colorToken(lines[5], baseColor, rainbow),
    });
}

// ── Achievement labels ────────────────────────────────────────────────────────
struct AchvInfo { std::string label; ftxui::Color color; bool rainbow; };
AchvInfo achievementFromParams(const SettlementRouteParams &p) {
    const bool isAP  = p.missCount == 0 && p.greatCount == 0;
    const bool isFC  = p.missCount == 0;
    const bool isULT = isAP && p.earlyCount == 0 && p.lateCount == 0;
    if (isULT) return {"ULT", ftxui::Color::White, true};
    if (isAP)  return {"AP",  ftxui::Color::RGB(255,215,0), false};
    if (isFC)  return {"FC",  ftxui::Color::RGB(192,192,192), false};
    return {};
}

// ── Component state ───────────────────────────────────────────────────────────
struct SettlementState {
    ThemePalette       palette;
    SettlementRouteParams params;
};

} // namespace

// ── Public factory ────────────────────────────────────────────────────────────
ftxui::Component GameplaySettlementUI::component(const SettlementRouteParams &params,
                                                  std::function<void(UIScene)> onRoute)
{
    using namespace ftxui;

    auto tr = [](const std::string &key) {
        return I18n::instance().get(key);
    };

    auto state = std::make_shared<SettlementState>();
    state->palette = ThemeAdapter::resolveFromRuntime();
    state->params  = params;

    auto routeBack = [state, onRoute = std::move(onRoute)]() mutable {
        onRoute(UIScene::ChartList);
    };

    auto root = Renderer([state, tr] {
        const SettlementRouteParams &p = state->params;
        const ThemePalette &pal = state->palette;

        // ── Grade block art ──────────────────────────────────────────────
        const GradeInfo grade = gradeFromAcc(p.accuracy);
        const AchvInfo  achv  = achievementFromParams(p);

        Elements artRows;
        artRows.push_back(
            hbox({filler(), blockArtWord(grade.label, grade.color, grade.rainbow), filler()}));
        if (!achv.label.empty()) {
            artRows.push_back(text(""));
            artRows.push_back(
                hbox({filler(), blockArtWord(achv.label, achv.color, achv.rainbow), filler()}));
        }
        Element artCol = vbox({
            filler(),
            vbox(std::move(artRows)),
            filler(),
        }) | size(WIDTH, EQUAL, 40);

        // ── Stats column (right side) ─────────────────────────────────────
        auto infoRow = [&](const std::string &labelKey, const std::string &val,
                           const Color &valCol = Color::Default) {
            return hbox({
                text(tr(labelKey)) | color(toColor(pal.textMuted)) | size(WIDTH, EQUAL, 18),
                filler(),
                text(val) | bold |
                    color(valCol == Color::Default ? toColor(pal.textPrimary) : valCol),
            });
        };

        std::ostringstream diffStr;
        diffStr << std::fixed << std::setprecision(1) << p.difficulty;

        const std::string scoreStr = fmtScore(p.score) + " / " + fmtScore(p.chartMaxScore);

        Element saveStatus;
        if (p.saveSucceeded) {
            saveStatus = text("✓ " + tr("ui.settlement.record_saved")) |
                         color(Color::RGB(100, 230, 100)) | bold;
        } else {
            saveStatus = text("✗ " + tr("ui.settlement.record_save_failed")) |
                         color(Color::RGB(255, 100, 100));
        }

        Element statsCol = vbox({
            infoRow("ui.settlement.score",     scoreStr,                          toColor(pal.accentPrimary)),
            infoRow("ui.settlement.accuracy",  fmtAcc(p.accuracy)),
            infoRow("ui.settlement.max_combo", "×" + std::to_string(p.maxCombo)),
            infoRow("ui.settlement.difficulty", diffStr.str()),
            separator(),
            hbox({
                text(tr("ui.settlement.perfect")) |
                    color(Color::RGB(255, 215, 0)) | size(WIDTH, EQUAL, 10),
                text(std::to_string(p.perfectCount)) | bold | size(WIDTH, EQUAL, 6),
                text(tr("ui.settlement.great")) |
                    color(Color::RGB(120, 200, 255)) | size(WIDTH, EQUAL, 10),
                text(std::to_string(p.greatCount)) | bold | size(WIDTH, EQUAL, 6),
                text(tr("ui.settlement.miss")) |
                    color(Color::RGB(255, 80, 80)) | size(WIDTH, EQUAL, 10),
                text(std::to_string(p.missCount)) | bold,
            }),
            hbox({
                text(tr("ui.settlement.early")) |
                    color(Color::RGB(120, 200, 255)) | size(WIDTH, EQUAL, 10),
                text(std::to_string(p.earlyCount)) | bold | size(WIDTH, EQUAL, 6),
                text(tr("ui.settlement.late")) |
                    color(Color::RGB(255, 180, 80)) | size(WIDTH, EQUAL, 10),
                text(std::to_string(p.lateCount)) | bold,
            }),
            text(""),
            saveStatus,
            filler(),
            hbox({filler(), text(tr("ui.settlement.hint")) | color(toColor(pal.textMuted))}),
        }) | flex;

        // ── Horizontal panel ──────────────────────────────────────────────
        Element panelBody = hbox({artCol, separator(), statsCol | flex});

        Element panel =
            window(text("  " + p.songName + "  "), panelBody) |
            color(toColor(pal.accentPrimary)) |
            bgcolor(toColor(pal.surfacePanel));

        // Dark background; panel vertically + horizontally centered
        return dbox({
            text("") | bgcolor(Color::RGB(10, 10, 15)) | flex,
            vbox({
                filler(),
                hbox({filler(), panel, filler()}),
                filler(),
            }) | flex,
            MessageOverlay::render(pal),
        }) |
        color(toColor(pal.textPrimary)) |
        flex;
    });

    return CatchEvent(root, [routeBack = std::move(routeBack)](const Event &event) mutable {
        if (event == Event::Return || event == Event::Escape ||
            event == Event::Character('q')) {
            routeBack();
            return true;
        }
        return false;
    });
}

} // namespace ui
