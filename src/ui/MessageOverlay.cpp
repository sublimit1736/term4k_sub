#include "ui/MessageOverlay.h"

#include "ui/UIColors.h"

#include <deque>
#include <mutex>

namespace ui {
namespace {

struct MessageItem {
    MessageLevel level = MessageLevel::Info;
    std::string text;
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
    queue().push_back({level, message});
}

bool MessageOverlay::hasPending() {
    std::scoped_lock lock(queueMutex());
    return !queue().empty();
}

bool MessageOverlay::handleEvent(const ftxui::Event &event) {
    if (event != ftxui::Event::Return && event != ftxui::Event::Escape &&
        event != ftxui::Event::Character('q')) {
        return false;
    }

    std::scoped_lock lock(queueMutex());
    if (queue().empty()) return false;
    queue().pop_front();
    return true;
}

ftxui::Element MessageOverlay::render(const ThemePalette &palette) {
    using namespace ftxui;

    MessageItem current;
    {
        std::scoped_lock lock(queueMutex());
        if (queue().empty()) return text("");
        current = queue().front();
    }

    const Element panel = vbox({
        text(current.text) | bold | color(levelColor(current.level)),
        separator(),
        text("Enter/Esc/q") | color(toColor(palette.textMuted)) | dim,
    }) |
    borderRounded |
    color(toColor(palette.accentPrimary)) |
    bgcolor(toColor(palette.surfacePanel)) |
    size(WIDTH, GREATER_THAN, 36);

    const Element shade = filler() | bgcolor(ftxui::Color::RGB(18, 18, 18));
    const Element popup = hbox({filler(), panel, filler()}) | vcenter;
    return dbox({shade, popup}) | flex;
}

} // namespace ui
