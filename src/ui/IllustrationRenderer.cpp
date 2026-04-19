// Suppress compiler warnings emitted by the stb single-header implementation.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4505)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "ui/IllustrationRenderer.h"
#include "ui/TerminalImageProtocol.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {
namespace {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Images are always decoded and scaled to this pixel size before further
/// processing.  Native-protocol output sends exactly this many pixels to the
/// terminal; half-block fallback downsamples from this size.
constexpr int kNativeImagePx = 180;

/// Number of bytes per pixel in 32-bit RGBA data.
constexpr int kBytesPerPixel = 4;

// ---------------------------------------------------------------------------
// Image cache
// ---------------------------------------------------------------------------

enum class CoverStatus {
    Unknown,    // not yet attempted
    NoFile,     // no cover image file found in the folder
    LoadFailed, // image file exists but could not be decoded
    Loaded      // decoded successfully
};

struct CachedCover {
    CoverStatus status = CoverStatus::Unknown;

    /// Complete Kitty APC sequence (all chunks) plus trailing space, ready to
    /// drop into an FTXUI screen cell.  Empty when no image protocol is
    /// available or when loading failed.
    std::string protocolPayload;

    /// Downsampled RGBA pixels for the half-block fallback renderer.
    /// Dimensions: kIllustrationCellW × (kIllustrationCellH * 2) × 4 bytes.
    std::vector<uint8_t> blockRgba;
};

std::unordered_map<std::string, CachedCover> g_coverCache;
std::mutex g_coverCacheMutex;

// ---------------------------------------------------------------------------
// File discovery
// ---------------------------------------------------------------------------

/// All candidate filenames tried in order.  stb_image will skip files that
/// it cannot decode (e.g., WebP without a plugin), so the list deliberately
/// covers common extensions used by rhythm-game chart packages.
static constexpr const char *kCoverCandidates[] = {
    "cover.jpg",   "cover.jpeg",  "cover.png",   "cover.bmp",
    "cover.JPG",   "cover.JPEG",  "cover.PNG",   "cover.BMP",
    "cover.webp",  "cover.WEBP",
    "jacket.jpg",  "jacket.jpeg", "jacket.png",
    "illustration.jpg", "illustration.jpeg", "illustration.png",
    "thumbnail.jpg", "thumbnail.png",
    "bg.jpg",      "bg.png",
    "art.jpg",     "art.png",
};

/// Returns the raw RGBA pixels for the first candidate file that both exists
/// on disk and can be decoded by stb_image.  Also returns the actual image
/// dimensions via \p outW / \p outH.  Returns an empty vector on failure.
std::vector<uint8_t> loadFirstDecodableImage(const std::string &folderPath,
                                             int &outW, int &outH) {
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const char *name : kCoverCandidates) {
        const fs::path p = fs::path(folderPath) / name;
        if (!fs::exists(p, ec) || !fs::is_regular_file(p, ec)) {
            ec.clear();
            continue;
        }
        int srcW = 0, srcH = 0, ch = 0;
        stbi_uc *raw = stbi_load(p.string().c_str(), &srcW, &srcH, &ch, kBytesPerPixel);
        if (!raw || srcW <= 0 || srcH <= 0) {
            if (raw) stbi_image_free(raw);
            continue; // try the next candidate
        }
        std::vector<uint8_t> pixels(raw, raw + srcW * srcH * kBytesPerPixel);
        stbi_image_free(raw);
        outW = srcW;
        outH = srcH;
        return pixels;
    }
    return {};
}

// ---------------------------------------------------------------------------
// Nearest-neighbour resize helper
// ---------------------------------------------------------------------------

void nnResize(const uint8_t *src, int srcW, int srcH,
              uint8_t       *dst, int dstW, int dstH) {
    for (int ty = 0; ty < dstH; ++ty) {
        for (int tx = 0; tx < dstW; ++tx) {
            const int sx = tx * srcW / dstW;
            const int sy = ty * srcH / dstH;
            const uint8_t *s = src + (sy * srcW + sx) * kBytesPerPixel;
            uint8_t       *d = dst + (ty * dstW + tx) * kBytesPerPixel;
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
        }
    }
}

// ---------------------------------------------------------------------------
// Load and cache a cover image
// ---------------------------------------------------------------------------

const CachedCover &loadCoverFromFolder(const std::string &folderPath) {
    std::lock_guard<std::mutex> lock(g_coverCacheMutex);

    auto it = g_coverCache.find(folderPath);
    if (it != g_coverCache.end() && it->second.status != CoverStatus::Unknown) {
        return it->second;
    }

    CachedCover &entry = g_coverCache[folderPath];

    // Try every candidate filename; skip silently if a file can't be decoded.
    int srcW = 0, srcH = 0;
    const std::vector<uint8_t> rawPixels = loadFirstDecodableImage(folderPath, srcW, srcH);
    if (rawPixels.empty()) {
        entry.status = CoverStatus::NoFile;
        return entry;
    }

    // Step 1 — scale to the fixed native pixel size (180 × 180).
    std::vector<uint8_t> native(static_cast<std::size_t>(kNativeImagePx * kNativeImagePx * kBytesPerPixel));
    nnResize(rawPixels.data(), srcW, srcH, native.data(), kNativeImagePx, kNativeImagePx);

    // Step 2 — build the terminal image protocol payload (if supported).
    if (TerminalImageProtocol::isSupported()) {
        entry.protocolPayload = TerminalImageProtocol::buildKittyPayload(
            native, kNativeImagePx, kNativeImagePx,
            kIllustrationCellW, kIllustrationCellH);
    }

    // Step 3 — downsample to block-art dimensions for the fallback renderer.
    // Each terminal row covers 2 vertical pixel rows (half-block characters).
    const int bW = kIllustrationCellW;
    const int bH = kIllustrationCellH * 2;
    entry.blockRgba.resize(static_cast<std::size_t>(bW * bH * kBytesPerPixel));
    nnResize(native.data(), kNativeImagePx, kNativeImagePx,
             entry.blockRgba.data(), bW, bH);

    entry.status = CoverStatus::Loaded;
    return entry;
}

// ---------------------------------------------------------------------------
// Custom FTXUI Node — native image protocol
// ---------------------------------------------------------------------------

/// A fixed-size FTXUI element that embeds a terminal image protocol payload.
///
/// The payload (a Kitty APC sequence + trailing space) is placed in the first
/// pixel of the reserved area.  All other pixels are set to space characters
/// so the layout occupies the expected terminal cells.
///
/// Why the trailing space in the payload?
///   APC sequences (ESC _ … ESC \) are control strings that do NOT advance
///   the terminal cursor.  FTXUI's Screen::ToString() outputs all cells of a
///   row sequentially without per-cell cursor-move instructions, relying on
///   natural cursor advancement.  Without the space the second cell would land
///   on the same column as the first, shifting every subsequent cell in that
///   row one position to the left.  The appended space makes the cursor
///   advance exactly one column — matching FTXUI's internal model.
class ImageProtocolNode : public ftxui::Node {
public:
    ImageProtocolNode(std::string payload, int cellW, int cellH)
        : payload_(std::move(payload)), cellW_(cellW), cellH_(cellH) {}

    void ComputeRequirement() override {
        requirement_.min_x = cellW_;
        requirement_.min_y = cellH_;
    }

    void Render(ftxui::Screen &screen) override {
        // Place the full sequence (APC + trailing space) in the first cell.
        screen.PixelAt(box_.x_min, box_.y_min).character = payload_;

        // Reserve all remaining cells in the image area with spaces so FTXUI
        // accounts for the correct number of columns and rows.
        for (int y = box_.y_min; y <= box_.y_max; ++y) {
            const int xStart = (y == box_.y_min) ? box_.x_min + 1 : box_.x_min;
            for (int x = xStart; x <= box_.x_max; ++x) {
                screen.PixelAt(x, y).character = " ";
            }
        }
    }

private:
    std::string payload_;
    int cellW_;
    int cellH_;
};

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ftxui::Element renderIllustration(const std::string &folderPath) {
    using namespace ftxui;

    if (!folderPath.empty()) {
        const CachedCover &cover = loadCoverFromFolder(folderPath);

        if (cover.status == CoverStatus::Loaded) {
            // ── Native protocol path ────────────────────────────────────────
            if (!cover.protocolPayload.empty()) {
                return std::make_shared<ImageProtocolNode>(
                    cover.protocolPayload, kIllustrationCellW, kIllustrationCellH);
            }

            // ── Half-block fallback ─────────────────────────────────────────
            // Each terminal row encodes two pixel rows: foreground = top pixel,
            // background = bottom pixel, using U+2580 UPPER HALF BLOCK (▀).
            const int pixW = kIllustrationCellW;
            const int pixH = kIllustrationCellH * 2;
            Elements rows;
            rows.reserve(static_cast<std::size_t>(kIllustrationCellH));
            for (int row = 0; row < kIllustrationCellH; ++row) {
                Elements cells;
                cells.reserve(static_cast<std::size_t>(kIllustrationCellW));
                for (int col = 0; col < kIllustrationCellW; ++col) {
                    const int topBase = ((row * 2)     * pixW + col) * kBytesPerPixel;
                    const int botBase = ((row * 2 + 1) * pixW + col) * kBytesPerPixel;
                    const Color topColor = Color::RGB(
                        cover.blockRgba[static_cast<std::size_t>(topBase)],
                        cover.blockRgba[static_cast<std::size_t>(topBase + 1)],
                        cover.blockRgba[static_cast<std::size_t>(topBase + 2)]);
                    const Color botColor = Color::RGB(
                        cover.blockRgba[static_cast<std::size_t>(botBase)],
                        cover.blockRgba[static_cast<std::size_t>(botBase + 1)],
                        cover.blockRgba[static_cast<std::size_t>(botBase + 2)]);
                    cells.push_back(text("▀") | color(topColor) | bgcolor(botColor));
                }
                rows.push_back(hbox(std::move(cells)));
            }
            return vbox(std::move(rows));
        }
    }

    // ── Placeholder ─────────────────────────────────────────────────────────
    const Color kDimColor = Color::RGB(70, 70, 70);
    return (text("?") | dim | color(kDimColor) | center)
           | borderRounded
           | color(kDimColor)
           | size(WIDTH,  EQUAL, kIllustrationCellW)
           | size(HEIGHT, EQUAL, kIllustrationCellH);
}

} // namespace ui
