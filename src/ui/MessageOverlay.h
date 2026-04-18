#pragma once

#include "ui/ThemeAdapter.h"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>

namespace ui {

enum class MessageLevel { Info, Warning, Error };

class MessageOverlay {
public:
    static void push(MessageLevel level, const std::string &message);

    static bool hasPending();

    // Returns true when the event is consumed to dismiss the current popup.
    static bool handleEvent(const ftxui::Event &event);

    // Returns an overlay element (or empty element) for dbox composition.
    static ftxui::Element render(const ThemePalette &palette);
};

} // namespace ui
