#pragma once

#include "ui/ThemeAdapter.h"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <functional>
#include <string>

namespace ui {

// A global right-click context menu overlay.
// Integrate by calling handleEvent() in every CatchEvent and render() in every dbox composition.
class ContextMenuOverlay {
public:
    // Update the text that "Copy" will put into the clipboard.
    // Typically call this with the current focused text or the active message text.
    static void setClipboardSource(const std::string &text);

    // Call to append additional lines to the clipboard source (e.g., pending message).
    static void setMessageText(const std::string &text);

    // Handle an FTXUI event.  Returns true when the event is consumed by the menu.
    // onPasteText: called with clipboard text when the user picks "Paste".
    static bool handleEvent(const ftxui::Event &event,
                            std::function<void(const std::string &)> onPasteText = nullptr);

    // Returns the overlay element (empty text element when menu is not visible).
    static ftxui::Element render(const ThemePalette &palette);

    static bool isVisible();
};

} // namespace ui
