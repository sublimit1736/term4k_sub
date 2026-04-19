#include "ui/ContextMenuOverlay.h"

#include "ui/UIColors.h"

#include <ftxui/dom/elements.hpp>

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace ui {
namespace {

// ─── Simple base64 encoder ───────────────────────────────────────────────────
static const char kB64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::string &input) {
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    const auto *data = reinterpret_cast<const unsigned char *>(input.data());
    const std::size_t n = input.size();
    for (std::size_t i = 0; i < n; i += 3) {
        const unsigned int b0 = data[i];
        const unsigned int b1 = (i + 1 < n) ? data[i + 1] : 0u;
        const unsigned int b2 = (i + 2 < n) ? data[i + 2] : 0u;
        out += kB64Table[(b0 >> 2) & 0x3F];
        out += kB64Table[((b0 << 4) | (b1 >> 4)) & 0x3F];
        out += (i + 1 < n) ? kB64Table[((b1 << 2) | (b2 >> 6)) & 0x3F] : '=';
        out += (i + 2 < n) ? kB64Table[b2 & 0x3F] : '=';
    }
    return out;
}

// Write text to the terminal clipboard via OSC 52 escape sequence.
void writeClipboard(const std::string &text) {
    const std::string encoded = base64Encode(text);
    // OSC 52 ; c (clipboard) ; <base64> ST
    std::fprintf(stdout, "\x1b]52;c;%s\a", encoded.c_str());
    std::fflush(stdout);
}

// ─── Menu state ──────────────────────────────────────────────────────────────

struct MenuState {
    bool visible      = false;
    int  mouseX       = 0;
    int  mouseY       = 0;
    int  selectedItem = 0;  // 0 = Copy, 1 = Paste hint
    std::string clipSource;   // text to copy
    std::string messageText;  // currently displayed message (if any)
};

std::mutex &stateMutex() {
    static std::mutex m;
    return m;
}
MenuState &state() {
    static MenuState s;
    return s;
}

// Build the list of menu items.
std::vector<std::string> buildItems(const MenuState &s) {
    std::vector<std::string> items;
    const std::string &copyText = s.messageText.empty() ? s.clipSource : s.messageText;
    if (!copyText.empty()) {
        // Show truncated preview
        const std::size_t kPreviewLen = 20;
        std::string preview = copyText.substr(0, kPreviewLen);
        if (copyText.size() > kPreviewLen) preview += "…";
        items.push_back("Copy: " + preview);
    } else {
        items.push_back("Copy");
    }
    items.push_back("Paste  (use Ctrl+Shift+V)");
    return items;
}

} // namespace

void ContextMenuOverlay::setClipboardSource(const std::string &text) {
    std::scoped_lock lock(stateMutex());
    state().clipSource = text;
}

void ContextMenuOverlay::setMessageText(const std::string &text) {
    std::scoped_lock lock(stateMutex());
    state().messageText = text;
}

bool ContextMenuOverlay::isVisible() {
    std::scoped_lock lock(stateMutex());
    return state().visible;
}

bool ContextMenuOverlay::handleEvent(const ftxui::Event &event) {
    std::scoped_lock lock(stateMutex());
    auto &s = state();

    // Right-click: open menu at cursor.
    // FTXUI v6 provides mouse() as a non-const member (no const overload).
    // The underlying Event object is always non-const; CatchEvent passes it as
    // const& only as a convention.  The cast is safe and read-only here.
    if (event.is_mouse()) {
        auto &mouse = const_cast<ftxui::Event &>(event).mouse();
        if (mouse.button == ftxui::Mouse::Right &&
            mouse.motion == ftxui::Mouse::Released) {
            s.visible      = true;
            s.mouseX       = mouse.x;
            s.mouseY       = mouse.y;
            s.selectedItem = 0;
            return true;
        }

        if (s.visible && mouse.button == ftxui::Mouse::Left &&
            mouse.motion == ftxui::Mouse::Released) {
            const auto items = buildItems(s);
            const int mx = mouse.x;
            const int my = mouse.y;
            const int itemY = my - s.mouseY;
            if (itemY >= 0 && itemY < static_cast<int>(items.size()) &&
                mx >= s.mouseX) {
                s.selectedItem = itemY;
                if (s.selectedItem == 0) {
                    const std::string &copyText = s.messageText.empty() ? s.clipSource : s.messageText;
                    if (!copyText.empty()) writeClipboard(copyText);
                }
            }
            s.visible = false;
            return true;
        }
    }

    if (!s.visible) return false;

    // Dismiss on Escape or 'q'.
    if (event == ftxui::Event::Escape || event == ftxui::Event::Character('q')) {
        s.visible = false;
        return true;
    }

    // Navigate menu items.
    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k')) {
        s.selectedItem = std::max(0, s.selectedItem - 1);
        return true;
    }
    if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j')) {
        const int itemCount = static_cast<int>(buildItems(s).size());
        s.selectedItem = std::min(itemCount - 1, s.selectedItem + 1);
        return true;
    }

    // Confirm selection with Enter.
    if (event == ftxui::Event::Return) {
        if (s.selectedItem == 0) {
            // Copy: write to terminal clipboard via OSC 52.
            const std::string &copyText = s.messageText.empty() ? s.clipSource : s.messageText;
            if (!copyText.empty()) writeClipboard(copyText);
        }
        // Item 1 = "Paste" hint — informs the user to use the terminal's native paste
        // (e.g. Ctrl+Shift+V / middle-click).  No programmatic clipboard read is
        // performed because OSC 52 read support varies widely across terminals.
        s.visible = false;
        return true;
    }

    return false;
}

ftxui::Element ContextMenuOverlay::render(const ThemePalette &palette) {
    using namespace ftxui;
    std::scoped_lock lock(stateMutex());
    const auto &s = state();
    if (!s.visible) return text("");

    const auto items = buildItems(s);

    Elements rows;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        Element row = text(" " + items[static_cast<std::size_t>(i)] + " ");
        if (i == s.selectedItem) {
            row = row | bold |
                  color(highContrastOn(palette.accentPrimary)) |
                  bgcolor(toColor(palette.accentPrimary));
        } else {
            row = row | color(toColor(palette.textPrimary));
        }
        rows.push_back(row);
    }

    Element menu = vbox(std::move(rows)) |
                   borderRounded |
                   color(toColor(palette.accentPrimary)) |
                   bgcolor(toColor(palette.surfacePanel));

    // Position menu relative to right-click location using a dbox trick:
    // pad with rows above and columns to the left.
    const int topPad  = std::max(0, s.mouseY - 1);
    const int leftPad = std::max(0, s.mouseX - 1);

    Element positioned = hbox({
        text(std::string(static_cast<std::size_t>(leftPad), ' ')),
        vbox({
            text(std::string(static_cast<std::size_t>(topPad), '\n')),
            menu,
            filler(),
        }),
        filler(),
    }) | flex;

    return positioned;
}

} // namespace ui
