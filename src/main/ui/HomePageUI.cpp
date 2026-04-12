#include "ui/HomePageUI.h"

#include "config/RuntimeConfigs.h"
#include "services/I18nService.h"
#include "ui/ThemeAdapter.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace ui {
namespace {

#ifndef TERM4K_VERSION
#define TERM4K_VERSION "dev"
#endif

ftxui::Color toColor(const Rgb &rgb) {
    return ftxui::Color::RGB(rgb.r, rgb.g, rgb.b);
}

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

} // namespace

int HomePageUI::run() {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) {
        return I18nService::instance().get(key);
    };

    ThemePalette palette = ThemeAdapter::resolveFromRuntime();
    auto screen = ScreenInteractive::Fullscreen();
    std::atomic<bool> ticking{true};
    int nextAction = 0;

    const std::array<std::string, 6> logo = {
        "████████╗███████╗██████╗ ███╗   ███╗██╗   ██╗██╗  ██╗",
        "╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║   ██║██║ ██╔╝",
        "   ██║   █████╗  ██████╔╝██╔████╔██║████████║█████╔╝ ",
        "   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║╚═════██║██╔═██╗ ",
        "   ██║   ███████╗██║  ██║██║ ╚═╝ ██║      ██║██║  ██╗",
        "   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝      ╚═╝╚═╝  ╚═╝",
    };

    std::vector<std::string> entries = {
        tr("ui.home.menu.start_game"),
        tr("ui.home.menu.settings"),
        tr("ui.home.menu.player_info"),
        tr("ui.home.menu.quit"),
    };
    const std::array<std::string, 4> icons = {"", "", "", ""};
    int selected = 0;

    auto root = Renderer([&] {
        Elements logoLines;
        for (const std::string &line : logo) {
            logoLines.push_back(text(line) | bold | color(toColor(palette.accentPrimary)) | center);
        }

        Elements menuRows;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            Element line = hbox({
                text(icons[i]),
                text("  "),
                text(entries[i]),
            });
            if (static_cast<int>(i) == selected) {
                line = line |
                       bold |
                       color(toColor(palette.surfaceBg)) |
                       bgcolor(toColor(palette.accentPrimary));
            } else {
                line = line | color(toColor(palette.textPrimary));
            }

            menuRows.push_back(hbox({filler(), line, filler()}));
            if (i + 1 < entries.size()) {
                // Two-line gap between options.
                menuRows.push_back(text(""));
                menuRows.push_back(text(""));
            }
        }

        Element menuArea = vbox(std::move(menuRows)) |
                           color(toColor(palette.textPrimary));

        Element menuPanel = hbox({text("   "), menuArea, text("   ")}) |
                            borderRounded |
                            color(toColor(palette.borderNormal));

        Element leftFooter = vbox({
                               text(std::string(TERM4K_VERSION)),
                               text(nowDateTimeString()),
                           }) |
                           color(toColor(palette.textMuted));

        Element rightFooter = vbox({
                                text("term4k") | bold | color(toColor(palette.accentPrimary)) | align_right,
                                text("github.com/TheBadRoger/term4k") | color(toColor(palette.textMuted)) | align_right,
                            });

        return vbox({
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
               bgcolor(toColor(palette.surfaceBg)) |
               color(toColor(palette.textPrimary)) |
               flex;
    });

    auto app = CatchEvent(root, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Escape) {
            ticking = false;
            screen.ExitLoopClosure()();
            return true;
        }

        if (event == Event::ArrowUp || event == Event::Character('k')) {
            selected = (selected + static_cast<int>(entries.size()) - 1) % static_cast<int>(entries.size());
            return true;
        }

        if (event == Event::ArrowDown || event == Event::Character('j')) {
            selected = (selected + 1) % static_cast<int>(entries.size());
            return true;
        }

        if (event == Event::Return) {
            if (selected == 0) {
                nextAction = 1;
                ticking = false;
                screen.ExitLoopClosure()();
                return true;
            }

            if (selected == 3) {
                ticking = false;
                screen.ExitLoopClosure()();
                return true;
            }
            return true;
        }

        return false;
    });

    std::thread ticker([&] {
        while (ticking) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!ticking) break;
            screen.PostEvent(Event::Custom);
        }
    });

    screen.Loop(app);
    ticking = false;
    if (ticker.joinable()) ticker.join();
    return nextAction;
}

} // namespace ui

