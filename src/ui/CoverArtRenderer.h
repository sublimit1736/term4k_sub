#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>

namespace ui {

/// Terminal columns reserved for the cover art block.
/// Matches a 180-pixel-wide image at the typical 8 px/cell font metric.
inline constexpr int kCoverArtCellW = 22;

/// Terminal rows reserved for the cover art block.
/// Matches a 180-pixel-tall image at the typical 16 px/cell font metric.
inline constexpr int kCoverArtCellH = 11;

/// Render cover art for the chart at @p folderPath.
///
/// Searches for cover.jpg / .jpeg / .png / .bmp / .webp in the folder.
/// Automatically selects the best available output method:
///   1. Kitty Graphics Protocol — 180 × 180 pixels in kCoverArtCellW ×
///      kCoverArtCellH terminal cells (native, pixel-perfect).
///   2. Unicode half-block (▀) with 24-bit RGB colour — block art fallback.
///   3. Dim '?' inside a rounded rectangle — if no cover image is found.
ftxui::Element renderCoverArt(const std::string &folderPath);

} // namespace ui
