#include "ui/MessageOverlay.h"

#include "ui/UIColors.h"

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

ftxui::Color levelColor(const MessageLevel level) {
    switch (level) {
        case MessageLevel::Info: return ftxui::Color::Default;
        case MessageLevel::Warning: return ftxui::Color::RGB(255, 176, 64);
        case MessageLevel::Error: return ftxui::Color::RGB(255, 96, 96);
    }
    return ftxui::Color::Default;
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

// Messages are auto-dismissed after 3 s; no key-press required.
bool MessageOverlay::handleEvent(const ftxui::Event &) {
    return false;
}

ftxui::Element MessageOverlay::render(const ThemePalette &palette) {
    using namespace ftxui;

    // Auto-dismiss expired messages (older than 3 s).
    {
        std::scoped_lock lock(queueMutex());
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
        if (queue().empty()) return text("");
    }

    MessageItem current;
    {
        std::scoped_lock lock(queueMutex());
        if (queue().empty()) return text("");
        current = queue().front();
    }

    const Element panel =
        text(current.text) | bold | color(levelColor(current.level)) |
        borderRounded |
        color(toColor(palette.accentPrimary)) |
        bgcolor(toColor(palette.surfacePanel)) |
        size(WIDTH, LESS_THAN, 44);

    // Top-right corner, no background dimming.
    return hbox({filler(), vbox({panel, filler()})}) | flex;
}

} // namespace ui
