#include "ui/LoadingUI.h"

#include "config/RuntimeConfigs.h"
#include "services/I18nService.h"
#include "ui/ThemeAdapter.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace ui {
    namespace {
        enum class SpinnerGlyphMode { FiraCode, Unicode, Ascii };

        SpinnerGlyphMode resolveSpinnerGlyphMode() {
            const char* env = std::getenv("TERM4K_LOADING_GLYPH");
            if (!env) return SpinnerGlyphMode::FiraCode;

            std::string value = env;
            for (char &ch: value){
                if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
            }

            if (value == "ascii") return SpinnerGlyphMode::Ascii;
            if (value == "unicode") return SpinnerGlyphMode::Unicode;
            return SpinnerGlyphMode::FiraCode;
        }

        const std::vector<std::string> &spinnerFrames(const SpinnerGlyphMode mode) {
            static const std::vector<std::string> kFiraFrames = {
                "", "", "", "", "", "",
            };
            static const std::vector<std::string> kUnicodeFrames = {
                "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏",
            };
            static const std::vector<std::string> kAsciiFrames = {
                "|", "/", "-", "\\",
            };

            switch (mode){
                case SpinnerGlyphMode::FiraCode: return kFiraFrames;
                case SpinnerGlyphMode::Unicode: return kUnicodeFrames;
                case SpinnerGlyphMode::Ascii: return kAsciiFrames;
            }
            return kUnicodeFrames;
        }
    } // namespace

    void LoadingUI::runUntilReady(ftxui::ScreenInteractive &screen,
                                  const std::function<bool()> &isReady,
                                          const std::string &labelKey,
                                          const int minDurationMs,
                                          const std::function<ftxui::Element()> &backgroundRenderer,
                                          const std::function<void()> &onShown
        ) {
        using namespace ftxui;

        I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
        const std::string loadingText = I18nService::instance().get(labelKey);
        const ThemePalette palette    = ThemeAdapter::resolveFromRuntime();

        const auto &frames     = spinnerFrames(resolveSpinnerGlyphMode());
        std::size_t frameIndex = 0;

        std::atomic ticking{true};
        const auto started = std::chrono::steady_clock::now();
        auto holder        = Container::Vertical({});

        auto root = Renderer(holder, [&] {
            const Element panel = hbox({
                                     text(frames[frameIndex % frames.size()]) | bold |
                                     color(toColor(palette.accentPrimary)),
                                     text("  "),
                                     text(loadingText) | bold | color(toColor(palette.textPrimary)),
                                 }) |
                                 borderRounded |
                                 color(toColor(palette.accentPrimary)) |
                                 bgcolor(toColor(palette.surfacePanel)) |
                                 size(WIDTH, GREATER_THAN, 30);

            const Element popup = hbox({filler(), panel, filler()}) | vcenter;
            if (backgroundRenderer){
                return dbox({backgroundRenderer() | dim | bgcolor(toColor(palette.surfacePanel)), popup}) | flex;
            }

            // Fallback dim strategy with unified backdrop color.
            const Element shade    = text(std::string(280, '.')) | color(Color::RGB(30, 30, 30));
            const Element backdrop = vbox({
                                              shade, shade, shade, shade, shade, shade, shade, shade, shade, shade,
                                              shade, shade, shade, shade, shade, shade, shade, shade, shade, shade
                                          }) |
                                     bgcolor(toColor(palette.surfacePanel)) |
                                     flex;
            return dbox({backdrop, popup}) | flex;
        });

        std::atomic startedPrepare{false};
        std::thread ticker([&] {
            while (ticking){
                if (!startedPrepare && onShown) {
                    onShown();
                    startedPrepare = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(90));
                if (!ticking) break;
                ++frameIndex;
                screen.PostEvent(Event::Custom);

                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - started
                    );
                if (elapsed.count() >= minDurationMs && isReady()){
                    ticking = false;
                    screen.ExitLoopClosure()();
                    break;
                }
            }
        });

        screen.Loop(root);
        ticking = false;
        if (ticker.joinable()) ticker.join();
    }
} // namespace ui

