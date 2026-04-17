#pragma once

#include <functional>
#include <string>

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

namespace ui {
    class LoadingUI {
    public:
        static void runUntilReady(ftxui::ScreenInteractive &screen,
                                  const std::function<bool()> &isReady,
                                  const std::string &labelKey                               = "ui.loading.text",
                                  int minDurationMs                                         = 160,
                                  const std::function<ftxui::Element()> &backgroundRenderer = {},
                                  const std::function<void()> &onShown                      = {}
            );
    };
} // namespace ui
