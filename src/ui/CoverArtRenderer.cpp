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

#include "ui/CoverArtRenderer.h"

#include <ftxui/dom/elements.hpp>

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {
namespace {

// ---------------------------------------------------------------------------
// Image cache
// ---------------------------------------------------------------------------

enum class CoverStatus {
    Unknown,    // not yet attempted
    NoFile,     // no cover image file found in folder
    LoadFailed, // image file exists but could not be decoded
    Loaded      // decoded successfully
};

struct CachedCover {
    CoverStatus status = CoverStatus::Unknown;
    std::vector<uint8_t> rgba; // decoded RGBA pixels, pixW × pixH × 4 bytes
    int pixW = 0;
    int pixH = 0;
};

std::unordered_map<std::string, CachedCover> g_coverCache;
std::mutex g_coverCacheMutex;

// ---------------------------------------------------------------------------
// File discovery
// ---------------------------------------------------------------------------

std::optional<std::filesystem::path> findCoverImageFile(const std::string &folderPath) {
    namespace fs = std::filesystem;
    static constexpr const char *kCandidates[] = {
        "cover.jpg",  "cover.jpeg",  "cover.png",  "cover.bmp",
        "cover.JPG",  "cover.JPEG",  "cover.PNG",  "cover.BMP",
        "cover.webp", "cover.WEBP",
    };
    std::error_code ec;
    for (const char *name : kCandidates) {
        fs::path p = fs::path(folderPath) / name;
        if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) return p;
        ec.clear();
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Load + resize into cache
// ---------------------------------------------------------------------------

// Returns a reference that is stable for the lifetime of g_coverCache.
const CachedCover &loadCoverFromFolder(const std::string &folderPath,
                                       const int targetW,
                                       const int targetH) {
    const std::string cacheKey =
        folderPath + '\0' + std::to_string(targetW) + 'x' + std::to_string(targetH);

    std::lock_guard<std::mutex> lock(g_coverCacheMutex);

    // Return existing entry if already resolved.
    auto it = g_coverCache.find(cacheKey);
    if (it != g_coverCache.end() && it->second.status != CoverStatus::Unknown) {
        return it->second;
    }

    CachedCover &entry = g_coverCache[cacheKey];

    // Search for a cover image file.
    const auto maybeFile = findCoverImageFile(folderPath);
    if (!maybeFile) {
        entry.status = CoverStatus::NoFile;
        return entry;
    }

    // Decode the image.
    int srcW, srcH, channels;
    stbi_uc *raw = stbi_load(maybeFile->string().c_str(), &srcW, &srcH, &channels, 4);
    if (!raw || srcW <= 0 || srcH <= 0) {
        entry.status = CoverStatus::LoadFailed;
        return entry;
    }

    // Nearest-neighbour resize to targetW × targetH.
    const std::size_t totalBytes = static_cast<std::size_t>(targetW * targetH * 4);
    entry.rgba.resize(totalBytes);
    for (int ty = 0; ty < targetH; ++ty) {
        for (int tx = 0; tx < targetW; ++tx) {
            const int sx = tx * srcW / targetW;
            const int sy = ty * srcH / targetH;
            const stbi_uc *src = raw + (sy * srcW + sx) * 4;
            uint8_t *dst = entry.rgba.data() + (ty * targetW + tx) * 4;
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
        }
    }
    stbi_image_free(raw);

    entry.pixW   = targetW;
    entry.pixH   = targetH;
    entry.status = CoverStatus::Loaded;
    return entry;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ftxui::Element renderCoverArt(const std::string &folderPath,
                               const int cellWidth,
                               const int cellHeight) {
    using namespace ftxui;

    // Each terminal row represents two pixel rows using the UPPER HALF BLOCK
    // character (▀): foreground = top pixel, background = bottom pixel.
    const int pixW = cellWidth;
    const int pixH = cellHeight * 2;

    if (!folderPath.empty()) {
        const CachedCover &cover = loadCoverFromFolder(folderPath, pixW, pixH);

        if (cover.status == CoverStatus::Loaded) {
            Elements rows;
            rows.reserve(static_cast<std::size_t>(cellHeight));
            for (int row = 0; row < cellHeight; ++row) {
                Elements cells;
                cells.reserve(static_cast<std::size_t>(cellWidth));
                for (int col = 0; col < cellWidth; ++col) {
                    const int topBase = ((row * 2)     * pixW + col) * 4;
                    const int botBase = ((row * 2 + 1) * pixW + col) * 4;
                    const Color topColor = Color::RGB(
                        cover.rgba[static_cast<std::size_t>(topBase)],
                        cover.rgba[static_cast<std::size_t>(topBase + 1)],
                        cover.rgba[static_cast<std::size_t>(topBase + 2)]);
                    const Color botColor = Color::RGB(
                        cover.rgba[static_cast<std::size_t>(botBase)],
                        cover.rgba[static_cast<std::size_t>(botBase + 1)],
                        cover.rgba[static_cast<std::size_t>(botBase + 2)]);
                    cells.push_back(text("\xe2\x96\x80") | color(topColor) | bgcolor(botColor));
                }
                rows.push_back(hbox(std::move(cells)));
            }
            return vbox(std::move(rows));
        }
    }

    // Placeholder: a rounded rectangle with a dim '?' in the centre.
    const Color kDimColor = Color::RGB(70, 70, 70);
    return (text("?") | dim | color(kDimColor) | center)
           | borderRounded
           | color(kDimColor)
           | size(WIDTH,  EQUAL, cellWidth)
           | size(HEIGHT, EQUAL, cellHeight);
}

} // namespace ui
