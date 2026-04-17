#include "ui/TransitionBackdrop.h"

#include <mutex>

namespace ui {
namespace {

ftxui::Element &lastFrame() {
    static auto frame = ftxui::text("");
    return frame;
}

std::mutex &frameMutex() {
    static std::mutex m;
    return m;
}

} // namespace

void TransitionBackdrop::update(const ftxui::Element &element) {
    std::scoped_lock lock(frameMutex());
    lastFrame() = element;
}

ftxui::Element TransitionBackdrop::render() {
    std::scoped_lock lock(frameMutex());
    return lastFrame();
}

} // namespace ui
