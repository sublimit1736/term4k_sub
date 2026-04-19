#include "ui/MessageOverlay.h"

#include "platform/I18n.h"
#include "platform/RuntimeConfig.h"
#include "ui/UIColors.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>

namespace ui {
namespace {

constexpr int kAutoDismissMs = 3000;

struct MessageItem {
    MessageLevel level = MessageLevel::Info;
    std::string text;
    std::chrono::steady_clock::time_point shownAt = std::chrono::steady_clock::now();
};

std::deque<MessageItem> &queue() {
    static std::deque<MessageItem> q;
    return q;
}

std::mutex &queueMutex() {
    static std::mutex m;
    return m;
}

// True while a mouse button is being held (possible text selection in progress).
std::atomic<bool> &mouseHeld() {
    static std::atomic<bool> h{false};
    return h;
}

ftxui::Color levelColor(const MessageLevel level) {
    switch (level) {
        case MessageLevel::Info: return ftxui::Color::Default;
        case MessageLevel::Warning: return ftxui::Color::RGB(255, 176, 64);
        case MessageLevel::Error: return ftxui::Color::RGB(255, 96, 96);
    }
    return ftxui::Color::Default;
}

// NerdFont icons (Font Awesome, available in Nerd Fonts patched terminals).
const char *levelIcon(const MessageLevel level) {
    switch (level) {
        case MessageLevel::Info:    return "\uF05A"; // nf-fa-info_circle
        case MessageLevel::Warning: return "\uF071"; // nf-fa-exclamation_triangle
        case MessageLevel::Error:   return "\uF057"; // nf-fa-times_circle
    }
    return "";
}

const char *levelI18nKey(const MessageLevel level) {
    switch (level) {
        case MessageLevel::Info:    return "ui.common.info";
        case MessageLevel::Warning: return "ui.common.warning";
        case MessageLevel::Error:   return "ui.common.error";
    }
    return "ui.common.info";
}

} // namespace

void MessageOverlay::push(const MessageLevel level, const std::string &message) {
    if (message.empty()) return;
    std::scoped_lock lock(queueMutex());
    queue().push_back({level, message, std::chrono::steady_clock::now()});
}

bool MessageOverlay::hasPending() {
    std::scoped_lock lock(queueMutex());
    return !queue().empty();
}

// Tracks mouse button presses to detect possible text selection.
bool MessageOverlay::handleEvent(const ftxui::Event &event) {
    if (event.is_mouse()) {
        // FTXUI v6 provides mouse() as a non-const member (no const overload).
        // The underlying Event object is always non-const; CatchEvent passes it as
        // const& only as a convention.  The cast is safe and read-only here.
        auto &mouse = const_cast<ftxui::Event &>(event).mouse();
        if (mouse.button == ftxui::Mouse::Left) {
            if (mouse.motion == ftxui::Mouse::Pressed) {
                mouseHeld().store(true, std::memory_order_relaxed);
            } else if (mouse.motion == ftxui::Mouse::Released) {
                if (mouseHeld().load(std::memory_order_relaxed)) {
                    mouseHeld().store(false, std::memory_order_relaxed);
                    // Reset dismiss timer for the front message — user may have just
                    // finished selecting text, so give them 3 more seconds to read it.
                    std::scoped_lock lock(queueMutex());
                    if (!queue().empty()) {
                        queue().front().shownAt = std::chrono::steady_clock::now();
                    }
                }
            }
        }
    }
    return false;
}

ftxui::Element MessageOverlay::render(const ThemePalette &palette) {
    using namespace ftxui;

    // Auto-dismiss expired messages (older than 3 s).
    // While mouse is held, skip dismissal so the user can select/read the text.
    {
        std::scoped_lock lock(queueMutex());
        if (!mouseHeld().load(std::memory_order_relaxed)) {
            const auto now = std::chrono::steady_clock::now();
            while (!queue().empty()) {
                const auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - queue().front().shownAt).count();
                if (elapsed >= kAutoDismissMs)
                    queue().pop_front();
                else
                    break;
            }
        }
        if (queue().empty()) return text("");
    }

    MessageItem current;
    {
        std::scoped_lock lock(queueMutex());
        if (queue().empty()) return text("");
        current = queue().front();
    }

    const std::string icon  = levelIcon(current.level);
    const std::string label = I18n::instance().get(levelI18nKey(current.level));
    const std::string fullText = icon + " " + label + ": " + current.text;

    // Inner text with explicit background to prevent underlying layers showing through.
    const Element innerText =
        paragraph(fullText) | bold | color(levelColor(current.level)) |
        bgcolor(toColor(palette.surfacePanel));

    const Element panel =
        innerText |
        borderRounded |
        color(toColor(palette.accentPrimary)) |
        bgcolor(toColor(palette.surfacePanel)) |
        size(WIDTH, LESS_THAN, 44);

    // Position the panel according to RuntimeConfig::toastPosition.
    const ToastPosition pos = RuntimeConfig::toastPosition;
    const bool atRight  = (pos == ToastPosition::TopRight  || pos == ToastPosition::BottomRight);
    const bool atBottom = (pos == ToastPosition::BottomLeft || pos == ToastPosition::BottomRight);

    Element hpositioned = atRight  ? hbox({filler(), panel}) : hbox({panel, filler()});
    Element vpositioned = atBottom ? vbox({filler(), hpositioned}) : vbox({hpositioned, filler()});
    return vpositioned | flex;
}

} // namespace ui
