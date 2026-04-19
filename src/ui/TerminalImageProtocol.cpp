#include "ui/TerminalImageProtocol.h"

#include <algorithm>  // std::min
#include <cstdlib>    // std::getenv
#include <cstring>    // std::strcmp
#include <string>

namespace ui {
namespace {

// ---------------------------------------------------------------------------
// Base64 encoding
// ---------------------------------------------------------------------------

std::string base64Encode(const uint8_t* data, std::size_t len) {
    static constexpr const char kAlpha[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (std::size_t i = 0; i < len; i += 3) {
        const uint32_t v =
              (static_cast<uint32_t>(data[i])             << 16)
            | (i + 1 < len ? static_cast<uint32_t>(data[i + 1]) << 8 : 0u)
            | (i + 2 < len ? static_cast<uint32_t>(data[i + 2])      : 0u);
        out += kAlpha[(v >> 18) & 0x3F];
        out += kAlpha[(v >> 12) & 0x3F];
        out += (i + 1 < len) ? kAlpha[(v >>  6) & 0x3F] : '=';
        out += (i + 2 < len) ? kAlpha[ v        & 0x3F] : '=';
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Protocol detection
// ---------------------------------------------------------------------------

ImageProtocol TerminalImageProtocol::detect() {
    static const ImageProtocol kCached = []() -> ImageProtocol {
        // Classic Kitty terminal sets KITTY_WINDOW_ID.
        if (std::getenv("KITTY_WINDOW_ID")) return ImageProtocol::Kitty;

        if (const char* term = std::getenv("TERM")) {
            if (std::strcmp(term, "xterm-kitty") == 0) return ImageProtocol::Kitty;
        }

        if (const char* tp = std::getenv("TERM_PROGRAM")) {
            // WezTerm and Ghostty both implement the Kitty Graphics Protocol.
            if (std::strcmp(tp, "WezTerm") == 0) return ImageProtocol::Kitty;
            if (std::strcmp(tp, "ghostty") == 0) return ImageProtocol::Kitty;
        }

        return ImageProtocol::None;
    }();
    return kCached;
}

bool TerminalImageProtocol::isSupported() {
    return detect() != ImageProtocol::None;
}

// ---------------------------------------------------------------------------
// Kitty Graphics Protocol sequence builder
// ---------------------------------------------------------------------------

std::string TerminalImageProtocol::buildKittyPayload(
        const std::vector<uint8_t>& rgba,
        int pixW, int pixH,
        int cellW, int cellH) {
    const std::string b64 = base64Encode(rgba.data(), rgba.size());

    std::string result;
    result.reserve(b64.size() + 512);

    // Kitty requires chunks of at most 4096 base64 characters.
    constexpr std::size_t kChunkSize = 4096;
    std::size_t pos = 0;
    bool first = true;

    while (pos < b64.size()) {
        const std::size_t end  = std::min(pos + kChunkSize, b64.size());
        const bool        last = (end == b64.size());

        result += "\033_G";
        if (first) {
            // a=T : transmit and display at current cursor position.
            // f=32: raw 32-bit RGBA pixel data.
            // s/v : source pixel dimensions.
            // c/r : display size in terminal cells (Kitty scales the image).
            // q=1 : suppress OK/error responses.
            result += "a=T,f=32,";
            result += "s=" + std::to_string(pixW) + ',';
            result += "v=" + std::to_string(pixH) + ',';
            result += "c=" + std::to_string(cellW) + ',';
            result += "r=" + std::to_string(cellH) + ',';
            result += "q=1,";
            first = false;
        }
        // m=1: more chunks follow; m=0: last chunk.
        result += 'm';
        result += last ? '0' : '1';
        result += ';';
        result.append(b64, pos, end - pos);
        result += "\033\\";
        pos = end;
    }

    // Append a single ASCII space.
    //
    // APC sequences (ESC _ … ESC \) are control strings and do NOT advance
    // the terminal cursor.  FTXUI's ToString() outputs cells sequentially
    // without per-cell cursor moves, relying on natural cursor advancement.
    // Without this space the next cell would be placed at the same column as
    // the APC cell, shifting all subsequent content in that row one column to
    // the left.  The space makes the cursor advance exactly one position,
    // matching FTXUI's expectation.
    result += ' ';
    return result;
}

} // namespace ui
