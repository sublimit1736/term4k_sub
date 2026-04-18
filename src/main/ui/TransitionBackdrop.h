#pragma once

#include <ftxui/dom/elements.hpp>

namespace ui {
    class TransitionBackdrop {
    public:
        static void update(const ftxui::Element &element);

        static ftxui::Element render();
    };
} // namespace ui
