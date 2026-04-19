#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>

namespace ui {

// Renders cover art from the chart folder as a terminal block-art element.
// Looks for cover.jpg / cover.jpeg / cover.png / cover.bmp in folderPath.
// If a cover image is found it is loaded, resized to cellWidth × (cellHeight*2)
// pixels, and rendered as rows of Unicode half-block (▀) characters with
// per-pixel 24-bit RGB colour.
// If no cover image is present a dim placeholder is rendered: a rounded
// rectangle the same size as the cover area with a centred dark '?' character.
// cellWidth  : desired width  in terminal character columns.
// cellHeight : desired height in terminal character rows.
ftxui::Element renderCoverArt(const std::string &folderPath, int cellWidth, int cellHeight);

} // namespace ui
