#pragma once

#include "ui/ThemeAdapter.h"

#include <ftxui/dom/elements.hpp>

namespace ui {

ftxui::Color toColor(const Rgb &rgb);
ftxui::Color highContrastOn(const Rgb &bg);

} // namespace ui
