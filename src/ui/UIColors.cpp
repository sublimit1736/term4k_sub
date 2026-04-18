#include "ui/UIColors.h"

#include <ftxui/screen/color.hpp>

namespace ui {

ftxui::Color toColor(const Rgb &rgb) {
    return ftxui::Color::RGB(rgb.r, rgb.g, rgb.b);
}

ftxui::Color highContrastOn(const Rgb &bg) {
    const int luma = (bg.r * 299 + bg.g * 587 + bg.b * 114) / 1000;
    return luma >= 140 ? ftxui::Color::Black : ftxui::Color::White;
}

} // namespace ui
