#include "ui/StartMenuUI.h"

#include "config/RuntimeConfigs.h"
#include "services/I18nService.h"
#include "ui/MessageOverlay.h"
#include "ui/TransitionBackdrop.h"
#include "ui/ThemeAdapter.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ui {
namespace {

#ifndef TERM4K_VERSION
#define TERM4K_VERSION "dev"
#endif


std::string nowDateTimeString() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &t);
#else
    local = *std::localtime(&t);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

struct StartMenuState {
    ThemePalette palette;
    std::vector<std::string> entries;
    int selected = 0;
    std::time_t lastClockSecond = 0;
    std::string cachedDateTime;
};

const std::array<std::string, 6> kLogo = {
    "████████╗███████╗██████╗ ███╗   ███╗██╗   ██╗██╗  ██╗",
    "╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║   ██║██║ ██╔╝",
    "   ██║   █████╗  ██████╔╝██╔████╔██║████████║█████╔╝ ",
    "   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║╚═════██║██╔═██╗ ",
    "   ██║   ███████╗██║  ██║██║ ╚═╝ ██║      ██║██║  ██╗",
    "   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝      ╚═╝╚═╝  ╚═╝",
};

const std::array<std::string, 4> kIcons = {"", "", "", ""};

} // namespace


ftxui::Component StartMenuUI::component(
    std::function<void(UIScene)> onRoute
    ) {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) {
        return I18nService::instance().get(key);
    };

    auto state = std::make_shared<StartMenuState>();
    state->palette = ThemeAdapter::resolveFromRuntime();
    state->entries = {
        tr("ui.home.menu.start_game"),
        tr("ui.home.menu.settings"),
        tr("ui.home.menu.player_info"),
        tr("ui.home.menu.quit"),
    };
    state->cachedDateTime = nowDateTimeString();

    auto updateClockCache = [state] {
        const std::time_t nowSec = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (nowSec == state->lastClockSecond) return;
        state->lastClockSecond = nowSec;
        state->cachedDateTime = nowDateTimeString();
    };

    auto root = Renderer([state, updateClockCache] {
        updateClockCache();
        Elements logoLines;
        for (const std::string &line : kLogo) {
            logoLines.push_back(text(line) | bold | color(toColor(state->palette.accentPrimary)) | center);
        }

        Elements menuRows;
        for (std::size_t i = 0; i < state->entries.size(); ++i) {
            Element line = hbox({
                text(kIcons[i]),
                text("  "),
                text(state->entries[i]),
            });
            if (static_cast<int>(i) == state->selected) {
                line = line |
                       bold |
                       color(highContrastOn(state->palette.accentPrimary)) |
                       bgcolor(toColor(state->palette.accentPrimary));
            } else {
                line = line | color(toColor(state->palette.textPrimary));
            }

            menuRows.push_back(hbox({filler(), line, filler()}));
            if (i + 1 < state->entries.size()) {
                // Two-line gap between options.
                menuRows.push_back(text(""));
                menuRows.push_back(text(""));
            }
        }

        Element menuArea = vbox(std::move(menuRows)) |
                           color(toColor(state->palette.textPrimary));

        Element menuPanel = hbox({text("   "), menuArea, text("   ")}) |
                            borderRounded |
                            color(toColor(state->palette.borderNormal));

        Element leftFooter = vbox({
                               text(std::string(TERM4K_VERSION)),
                               text(state->cachedDateTime),
                           }) |
                           color(toColor(state->palette.textMuted));

        Element rightFooter = vbox({
                                text("term4k") | bold | color(toColor(state->palette.accentPrimary)) | align_right,
                                text("github.com/TheBadRoger/term4k") | color(toColor(state->palette.textMuted)) | align_right,
                            });

        Element base = vbox({
                   // Move logo closer to 1/4 of the screen height.
                   filler() | flex,
                   vbox(std::move(logoLines)),
                   text(""),
                   text(""),
                   text(""),
                   text(""),
                   hbox({filler(), menuPanel, filler()}),
                   filler() | flex,
                   hbox({leftFooter, filler(), rightFooter}),
               }) |
               bgcolor(toColor(state->palette.surfaceBg)) |
               color(toColor(state->palette.textPrimary)) |
               flex;

        Element composed = dbox({base, MessageOverlay::render(state->palette)});
        TransitionBackdrop::update(composed);
        return composed;
    });

    auto app = CatchEvent(root, [state, onRoute = std::move(onRoute)](const Event &event) {
        if (MessageOverlay::handleEvent(event)) {
            return true;
        }

        if (event == Event::Character('q') || event == Event::Escape) {
            onRoute(UIScene::Exit);
            return true;
        }

        if (event == Event::ArrowUp || event == Event::Character('k')) {
            state->selected = (state->selected + static_cast<int>(state->entries.size()) - 1) % static_cast<int>(state->entries.size());
            return true;
        }

        if (event == Event::ArrowDown || event == Event::Character('j')) {
            state->selected = (state->selected + 1) % static_cast<int>(state->entries.size());
            return true;
        }

        if (event == Event::Return) {
            if (state->selected == 0) {
                onRoute(UIScene::ChartList);
                return true;
            }

            if (state->selected == 1) {
                onRoute(UIScene::Settings);
                return true;
            }

            if (state->selected == 2) {
                onRoute(UIScene::UserStat);
                return true;
            }

            if (state->selected == 3) {
                onRoute(UIScene::Exit);
                return true;
            }
            return true;
        }

        return false;
    });

    return app;
}

} // namespace ui
