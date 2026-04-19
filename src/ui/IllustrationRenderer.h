#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>

namespace ui {

/// Terminal columns reserved for the illustration block.
/// Matches a 180-pixel-wide image at the typical 8 px/cell font metric.
inline constexpr int kIllustrationCellW = 22;

/// Terminal rows reserved for the illustration block.
/// Matches a 180-pixel-tall image at the typical 16 px/cell font metric.
inline constexpr int kIllustrationCellH = 11;

/// Render the illustration (cover art) for the chart at @p folderPath.
///
/// Searches for cover.jpg / .jpeg / .png / .bmp / .webp in the folder.
/// Automatically selects the best available output method:
///   1. Kitty Graphics Protocol — 180 × 180 pixels in kIllustrationCellW ×
///      kIllustrationCellH terminal cells (native, pixel-perfect).
///   2. Unicode half-block (▀) with 24-bit RGB colour — block art fallback.
///   3. Dim '?' inside a rounded rectangle — if no cover image is found.
ftxui::Element renderIllustration(const std::string &folderPath);

} // namespace ui
