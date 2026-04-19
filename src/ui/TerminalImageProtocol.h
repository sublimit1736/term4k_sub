#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ui {

/// Terminal image protocols supported by term4k.
enum class ImageProtocol {
    None,   ///< No image protocol detected; use half-block art fallback.
    Kitty,  ///< Kitty Graphics Protocol (xterm-kitty / KITTY_WINDOW_ID / WezTerm / Ghostty).
};

/// Detection and encoding helpers for mainstream terminal image protocols.
class TerminalImageProtocol {
public:
    /// Returns the best image protocol supported by the current terminal.
    /// The result is computed once and cached for the process lifetime.
    static ImageProtocol detect();

    /// Returns true if any image protocol is available.
    static bool isSupported();

    /// Builds a Kitty Graphics Protocol multi-chunk APC sequence that
    /// transmits and displays the given 32-bit RGBA image at the current
    /// cursor position.
    ///
    /// @param rgba   Raw RGBA pixels (pixW × pixH × 4 bytes).
    /// @param pixW   Source image pixel width.
    /// @param pixH   Source image pixel height.
    /// @param cellW  Terminal columns the image should occupy.
    /// @param cellH  Terminal rows the image should occupy.
    ///
    /// @return A string containing all APC chunks plus a trailing U+0020
    ///         SPACE character.  The space is required so that the terminal
    ///         cursor advances exactly one column after the non-advancing APC
    ///         sequence, keeping FTXUI's subsequent cell output aligned.
    static std::string buildKittyPayload(const std::vector<uint8_t>& rgba,
                                         int pixW, int pixH,
                                         int cellW, int cellH);
};

} // namespace ui
