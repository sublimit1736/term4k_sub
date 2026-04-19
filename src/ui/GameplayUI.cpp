#include "ui/GameplayUI.h"

#include "platform/RuntimeConfig.h"
#include "data/GameplayChartData.h"
#include "gameplay/GameplaySession.h"
#include "scenes/GameplaySettlement.h"
#include "audio/AudioPlayer.h"
#include "gameplay/GameplayChartParser.h"
#include "platform/I18n.h"
#include "ui/MessageOverlay.h"
#include "ui/ThemeAdapter.h"
#include "ui/UIColors.h"
#include "account/UserContext.h"
#include "utils/ErrorNotifier.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ui {
namespace {

// ── Display constants ─────────────────────────────────────────────────────────
constexpr int      kFieldHeight      = 20;    // Rows in the note-fall area
constexpr uint32_t kDisplayWindowMs  = 1400;  // How far ahead notes are shown
constexpr uint32_t kKeyHoldTimeoutMs = 320;   // Auto key-up after no key-repeat

// ── Formatting helpers ────────────────────────────────────────────────────────
std::string formatScore(const uint64_t score) {
    std::string s = std::to_string(score);
    // Insert thousand separators
    int insertPos = static_cast<int>(s.size()) - 3;
    while (insertPos > 0) {
        s.insert(static_cast<std::size_t>(insertPos), ",");
        insertPos -= 3;
    }
    return s;
}

std::string formatAccuracy(const double acc) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << (acc * 100.0) << "%";
    return out.str();
}

std::string formatMs(const uint32_t ms) {
    const uint32_t secs  = ms / 1000;
    const uint32_t mins  = secs / 60;
    const uint32_t s     = secs % 60;
    std::ostringstream out;
    out << mins << ":" << std::setw(2) << std::setfill('0') << s;
    return out.str();
}

// Key code ↔ printable label
std::string keyLabel(const uint8_t keyCode) {
    if (keyCode >= 32 && keyCode < 127) return std::string(1, static_cast<char>(keyCode));
    return "?";
}

// ── Note-field cell types ─────────────────────────────────────────────────────
enum class CellType { Empty, TapNote, HoldHead, HoldBody, HoldTail };

struct NoteCell {
    CellType type        = CellType::Empty;
    bool     lanePressed = false;
    // Sub-row precision level for tap notes (1–8).
    // Maps to the lower-block Unicode character ▁▂▃▄▅▆▇█.
    // The TOP EDGE of the filled region represents the note's position within
    // the character cell: level 8 (█) = top of cell, level 1 (▁) = near bottom.
    int subLevel = 8;
};

// Lower-block characters by fill level (index 0 = empty / not used for notes,
// 1–8 = ▁ ▂ ▃ ▄ ▅ ▆ ▇ █).
static const char* const kLowerBlockUtf8[] = {
    nullptr,  // 0: unused
    "▁",      // U+2581  lower one-eighth block
    "▂",      // U+2582  lower one-quarter block
    "▃",      // U+2583  lower three-eighths block
    "▄",      // U+2584  lower half block
    "▅",      // U+2585  lower five-eighths block
    "▆",      // U+2586  lower three-quarters block
    "▇",      // U+2587  lower seven-eighths block
    "█",      // U+2588  full block
};

// Build a 4-column-wide tap-note bar string for the given sub-level.
std::string tapNoteBar(const int subLevel) {
    std::string bar;
    bar.reserve(4 * 4);  // up to 4 bytes per UTF-8 char × 4 columns
    const char* blk = kLowerBlockUtf8[subLevel];
    for (int i = 0; i < 4; ++i) bar += blk;
    return bar;
}

// Render a single note-field cell (4 chars wide).
ftxui::Element renderCell(const NoteCell &cell, const ThemePalette &palette) {
    using namespace ftxui;

    constexpr int kCellW = 4;

    if (cell.type == CellType::TapNote) {
        const std::string bar = tapNoteBar(cell.subLevel);
        Element e = text(bar) | color(toColor(palette.accentPrimary)) | bold;
        if (cell.lanePressed) e = e | bgcolor(toColor(palette.accentPrimary)) | color(highContrastOn(palette.accentPrimary));
        return e | size(WIDTH, EQUAL, kCellW);
    }
    if (cell.type == CellType::HoldHead) {
        return text("╠══╣") |
               color(Color::RGB(100, 200, 255)) | bold |
               size(WIDTH, EQUAL, kCellW);
    }
    if (cell.type == CellType::HoldBody) {
        return text("║  ║") |
               color(Color::RGB(60, 140, 220)) |
               size(WIDTH, EQUAL, kCellW);
    }
    if (cell.type == CellType::HoldTail) {
        return text("╠══╣") |
               color(Color::RGB(60, 180, 200)) | bold |
               size(WIDTH, EQUAL, kCellW);
    }
    // Empty cell
    const Color bg = cell.lanePressed
                         ? Color::RGB(50, 50, 70)
                         : Color::RGB(20, 20, 30);
    return text("    ") | bgcolor(bg) | size(WIDTH, EQUAL, kCellW);
}

// ── Build the note-field grid ─────────────────────────────────────────────────
// Returns grid[row][lane]. row 0 = top (far future), row kFieldHeight-1 = hit zone.
std::vector<std::vector<NoteCell>> buildNoteGrid(
    const GameplayChartData &chartData,
    const uint32_t            displayTimeMs,
    const int                 keyCount,
    const std::vector<bool>  &lanePressed)
{
    std::vector<std::vector<NoteCell>> grid(
        kFieldHeight, std::vector<NoteCell>(static_cast<std::size_t>(keyCount)));

    for (int r = 0; r < kFieldHeight; ++r) {
        for (int l = 0; l < keyCount; ++l) {
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(l)].lanePressed =
                lanePressed[static_cast<std::size_t>(l)];
        }
    }

    const double msPerRow =
        static_cast<double>(kDisplayWindowMs) / std::max(kFieldHeight - 1, 1);

    // Helper: time-delta in ms → exact floating-point row position.
    // Row 0 = top (far future, delta = kDisplayWindowMs),
    // row kFieldHeight-1 = bottom (present, delta = 0).
    auto exactRowFor = [&](const double deltaMs) -> double {
        return (kFieldHeight - 1) * (1.0 - deltaMs / kDisplayWindowMs);
    };

    // Helper: time-delta → integer row using floor (sub-row precision for tap notes).
    auto rowFloor = [&](const double deltaMs) -> int {
        return static_cast<int>(std::floor(exactRowFor(deltaMs)));
    };

    // Helper: time-delta → integer row using round (used for hold head/tail).
    auto rowRound = [&](const double deltaMs) -> int {
        return static_cast<int>(std::lround(exactRowFor(deltaMs)));
    };

    // Helper: time-delta → sub-row level in [1, 8].
    // frac = fractional distance of the note from the top of its character cell.
    // level 8 (█) = note at top of cell; level 1 (▁) = note near bottom of cell.
    // The TOP EDGE of the lower-block character sits at exactly the note's position.
    auto subLevelFor = [&](const double deltaMs) -> int {
        const double r    = exactRowFor(deltaMs);
        const double frac = r - std::floor(r);  // 0.0 (top of cell) .. <1.0 (bottom)
        const int level   = static_cast<int>(std::round(8.0 * (1.0 - frac)));
        return std::max(1, std::min(8, level));
    };

    // ── Tap notes ──
    for (const auto &tap : chartData.getTaps()) {
        const int lane = static_cast<int>(tap.getLane());
        if (lane < 0 || lane >= keyCount) continue;

        const double delta = static_cast<double>(tap.getTimeMs()) -
                             static_cast<double>(displayTimeMs);
        if (delta < -msPerRow || delta > kDisplayWindowMs + msPerRow) continue;

        const int row = rowFloor(delta);
        if (row < 0 || row >= kFieldHeight) continue;

        auto &cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(lane)];
        if (cell.type == CellType::Empty) {
            cell.type     = CellType::TapNote;
            cell.subLevel = subLevelFor(delta);
        }
    }

    // ── Hold notes ──
    for (const auto &hold : chartData.getHolds()) {
        const int lane = static_cast<int>(hold.getLane());
        if (lane < 0 || lane >= keyCount) continue;

        const double headDelta = static_cast<double>(hold.getHeadTimeMs()) -
                                 static_cast<double>(displayTimeMs);
        const double tailDelta = static_cast<double>(hold.getTailTimeMs()) -
                                 static_cast<double>(displayTimeMs);

        if (headDelta > kDisplayWindowMs + msPerRow) continue;
        if (tailDelta < -msPerRow)                   continue;

        const int headRow = rowRound(headDelta);
        const int tailRow = rowRound(tailDelta);

        // headRow < tailRow because head is further in future
        const int topRow = std::min(headRow, tailRow);
        const int botRow = std::max(headRow, tailRow);

        for (int row = std::max(0, topRow); row <= std::min(kFieldHeight - 1, botRow); ++row) {
            auto &cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(lane)];
            if (row == headRow)      cell.type = CellType::HoldHead;
            else if (row == tailRow) cell.type = CellType::HoldTail;
            else if (cell.type == CellType::Empty) cell.type = CellType::HoldBody;
        }
    }

    return grid;
}

// ── Render the note-fall field ────────────────────────────────────────────────
ftxui::Element renderNoteField(
    const GameplayChartData &chartData,
    const uint32_t           displayTimeMs,
    const int                keyCount,
    const std::vector<bool> &lanePressed,
    const ThemePalette      &palette,
    const GameplaySnapshot  &snap)
{
    using namespace ftxui;

    const auto grid = buildNoteGrid(chartData, displayTimeMs, keyCount, lanePressed);

    Elements rows;
    rows.reserve(static_cast<std::size_t>(kFieldHeight + 2));

    // Note rows (top to bottom)
    for (int r = 0; r < kFieldHeight; ++r) {
        Elements cells;
        cells.push_back(text("│") | color(toColor(palette.borderNormal)));
        for (int l = 0; l < keyCount; ++l) {
            cells.push_back(
                renderCell(grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(l)], palette));
            cells.push_back(text("│") | color(toColor(palette.borderNormal)));
        }
        rows.push_back(hbox(std::move(cells)));
    }

    // Hit line
    {
        Elements cells;
        cells.push_back(text("╠") | color(toColor(palette.accentPrimary)));
        for (int l = 0; l < keyCount; ++l) {
            const bool pressed = lanePressed[static_cast<std::size_t>(l)];
            Element seg = text("════") |
                          color(pressed ? highContrastOn(palette.accentPrimary)
                                        : toColor(palette.accentPrimary));
            if (pressed) seg = seg | bgcolor(toColor(palette.accentPrimary));
            cells.push_back(seg | size(WIDTH, EQUAL, 4));
            if (l + 1 < keyCount)
                cells.push_back(text("╬") | color(toColor(palette.accentPrimary)));
        }
        cells.push_back(text("╣") | color(toColor(palette.accentPrimary)));
        rows.push_back(hbox(std::move(cells)));
    }

    // Key label row
    {
        const auto &bindings = RuntimeConfig::keyBindings;
        Elements cells;
        cells.push_back(text("│") | color(toColor(palette.borderNormal)));
        for (int l = 0; l < keyCount; ++l) {
            const uint8_t kc = (l < static_cast<int>(bindings.size())) ? bindings[static_cast<std::size_t>(l)] : 0;
            const bool    pressed = lanePressed[static_cast<std::size_t>(l)];
            std::string lbl = " " + keyLabel(kc) + "  ";
            lbl.resize(4, ' ');
            Element cell = text(lbl) | bold;
            if (pressed) {
                cell = cell | bgcolor(toColor(palette.accentPrimary)) |
                              color(highContrastOn(palette.accentPrimary));
            } else {
                cell = cell | color(toColor(palette.textPrimary));
            }
            cells.push_back(cell | size(WIDTH, EQUAL, 4));
            cells.push_back(text("│") | color(toColor(palette.borderNormal)));
        }
        rows.push_back(hbox(std::move(cells)));
    }

    // Top border
    Elements borderRow;
    borderRow.push_back(text("╔") | color(toColor(palette.borderNormal)));
    for (int l = 0; l < keyCount; ++l) {
        borderRow.push_back(text("════") | color(toColor(palette.borderNormal)) | size(WIDTH, EQUAL, 4));
        borderRow.push_back(text(l + 1 < keyCount ? "╦" : "╗") |
                             color(toColor(palette.borderNormal)));
    }

    // Combo flash: AP / FC indicator
    (void)snap; // snap is used for stats panel; field itself doesn't need it further

    Element field = vbox(std::move(rows));
    return vbox({hbox(std::move(borderRow)), field});
}

// ── Stats panel ───────────────────────────────────────────────────────────────
ftxui::Element renderStatsPanel(
    const GameplaySnapshot &snap,
    const ThemePalette     &palette,
    const std::function<std::string(const std::string &)> &tr,
    const uint32_t           totalDurationMs,
    const bool               showEarlyLate,
    const bool               showAP,
    const bool               showFC)
{
    using namespace ftxui;

    const uint64_t maxScore = snap.getChartMaxScore();
    const uint64_t score    = snap.getScore();

    // Score row
    Element scoreElem =
        vbox({
            text(tr("ui.gameplay.score")) |
                color(toColor(palette.textMuted)) | bold,
            text(formatScore(score)) |
                color(toColor(palette.accentPrimary)) | bold |
                size(WIDTH, EQUAL, 14),
        }) | size(WIDTH, EQUAL, 18);

    // Accuracy
    Element accElem =
        hbox({
            text(tr("ui.gameplay.accuracy") + " ") |
                color(toColor(palette.textMuted)),
            text(formatAccuracy(snap.getAccuracy())) |
                color(toColor(palette.textPrimary)) | bold,
        });

    // Combo
    const uint32_t combo    = snap.getCurrentCombo();
    const uint32_t maxCombo = snap.getMaxCombo();
    const bool     isAP     = snap.getMissCount() == 0 && snap.getGreatCount() == 0;
    const bool     isFC     = snap.getMissCount() == 0;

    Color comboColor = toColor(palette.textPrimary);
    if (isAP && showAP)   comboColor = Color::RGB(255, 215, 0);
    else if (isFC && showFC) comboColor = Color::RGB(192, 192, 192);

    Element comboElem =
        hbox({
            text(tr("ui.gameplay.combo") + " ") |
                color(toColor(palette.textMuted)),
            text("×" + std::to_string(combo)) |
                color(comboColor) | bold,
            text("  " + tr("ui.gameplay.max_combo") + " ×" + std::to_string(maxCombo)) |
                color(toColor(palette.textMuted)),
        });

    // AP / FC badge
    Elements badges;
    if (showAP && isAP) {
        badges.push_back(text(" AP ") | bold |
                         color(Color::RGB(255, 215, 0)) |
                         bgcolor(Color::RGB(40, 30, 0)));
        badges.push_back(text("  "));
    }
    if (showFC && isFC && !isAP) {
        badges.push_back(text(" FC ") | bold |
                         color(Color::RGB(192, 192, 192)) |
                         bgcolor(Color::RGB(30, 30, 30)));
        badges.push_back(text("  "));
    }

    // Judgement counts
    auto judgRow = [&](const std::string &labelKey, const uint32_t count, const Color &col) {
        return hbox({
            text(tr(labelKey)) | color(toColor(palette.textMuted)) | size(WIDTH, EQUAL, 10),
            text(std::to_string(count)) | color(col) | bold | size(WIDTH, EQUAL, 6),
        });
    };

    Element judgePanel = vbox({
        judgRow("ui.gameplay.perfect", snap.getPerfectCount(), Color::RGB(255, 215, 0)),
        judgRow("ui.gameplay.great",   snap.getGreatCount(),   Color::RGB(120, 200, 255)),
        judgRow("ui.gameplay.miss",    snap.getMissCount(),    Color::RGB(255, 80, 80)),
    });

    // Early / Late
    Elements earlyLateRows;
    if (showEarlyLate) {
        earlyLateRows.push_back(
            hbox({
                text(tr("ui.gameplay.early")) | color(Color::RGB(120, 200, 255)) |
                    size(WIDTH, EQUAL, 8),
                text(std::to_string(snap.getEarlyCount())) |
                    color(toColor(palette.textPrimary)) | bold,
                text("  "),
                text(tr("ui.gameplay.late")) | color(Color::RGB(255, 180, 80)) |
                    size(WIDTH, EQUAL, 8),
                text(std::to_string(snap.getLateCount())) |
                    color(toColor(palette.textPrimary)) | bold,
            }));
    }

    // Progress bar
    const uint32_t timeMs = snap.getCurrentChartTimeMs();
    const int      pct    = (totalDurationMs > 0)
                              ? static_cast<int>(std::min<uint64_t>(timeMs, totalDurationMs) * 100 /
                                                  totalDurationMs)
                              : 0;
    const int      filled = pct * 24 / 100;
    std::string    bar    = "[";
    for (int i = 0; i < 24; ++i) bar += (i < filled ? "█" : "░");
    bar += "] " + formatMs(timeMs) + " / " + formatMs(totalDurationMs);

    // Score bar vs max score
    const int scorePct = (maxScore > 0)
                             ? static_cast<int>(std::min<uint64_t>(score, maxScore) * 100 / maxScore)
                             : 0;
    const int sBar = scorePct * 16 / 100;
    std::string scoreBar;
    for (int i = 0; i < 16; ++i) scoreBar += (i < sBar ? "▮" : "▯");

    Element statsContent = vbox({
        scoreElem,
        text(scoreBar) | color(toColor(palette.accentPrimary)),
        text(""),
        accElem,
        comboElem,
        text(""),
        badges.empty() ? text("") : hbox(std::move(badges)),
        text(""),
        judgePanel,
        text(""),
        vbox(std::move(earlyLateRows)),
        filler(),
        text(bar) | color(toColor(palette.textMuted)),
    });

    return window(
        text("  " + tr("ui.gameplay.stats_title") + " "),
        statsContent) |
        color(toColor(palette.accentPrimary)) |
        bgcolor(toColor(palette.surfacePanel)) |
        flex;
}

// ── Top info bar ──────────────────────────────────────────────────────────────
ftxui::Element renderTopBar(
    const std::string  &songName,
    const float         difficulty,
    const int           keyCount,
    const ThemePalette &palette,
    const std::function<std::string(const std::string &)> &tr)
{
    using namespace ftxui;
    std::ostringstream diffStr;
    diffStr << std::fixed << std::setprecision(1) << difficulty;

    return hbox({
        text(songName) | bold | color(toColor(palette.textPrimary)) | flex,
        text(tr("ui.gameplay.difficulty") + " " + diffStr.str()) |
            color(toColor(palette.textMuted)),
        text("  "),
        text(std::to_string(keyCount) + tr("ui.gameplay.keys")) |
            color(toColor(palette.textMuted)),
    });
}

// ── Bottom hint bar ───────────────────────────────────────────────────────────
ftxui::Element renderBottomBar(
    const ThemePalette &palette,
    const std::function<std::string(const std::string &)> &tr)
{
    using namespace ftxui;
    return hbox({
        text(tr("ui.gameplay.hint")) | color(toColor(palette.textMuted)),
        filler(),
    }) | size(HEIGHT, EQUAL, 1);
}

// ── Component state ───────────────────────────────────────────────────────────
struct GameplayState {
    ThemePalette     palette;
    GameplaySession gameplay;
    AudioPlayer     audio;
    GameplayChartData chartData;

    GameplayRouteParams params;

    std::vector<bool> lanePressed;   // per-lane press indicator for visuals
    std::unordered_map<uint8_t, std::chrono::steady_clock::time_point> heldKeys;

    bool    audioStarted    = false;
    bool    resultTriggered = false;
    bool    routed          = false;
    uint32_t totalDurationMs = 0;

    std::atomic<bool> bgRunning{false};
    std::thread       bgThread;

    ~GameplayState() {
        bgRunning.store(false);
        if (bgThread.joinable()) bgThread.join();
        audio.stopSong();
    }
};

} // namespace

// ── Public factory ────────────────────────────────────────────────────────────
ftxui::Component GameplayUI::component(ftxui::ScreenInteractive &screen,
                                       const GameplayRouteParams &params,
                                       std::function<void(UIScene)> onRoute)
{
    using namespace ftxui;

    auto tr = [](const std::string &key) {
        return I18n::instance().get(key);
    };

    auto state = std::make_shared<GameplayState>();
    state->palette = ThemeAdapter::resolveFromRuntime();
    state->params  = params;

    // Load chart data for note-field rendering.
    // Intercept ErrorNotifier messages during parsing to detect out-of-range notes (W1).
    bool hadOutOfRangeNotes = false;
    {
        // Capture prevSink by value so it remains valid if the lambda outlives this scope.
        auto prevSink = ErrorNotifier::getSink();
        // Match against the translated out-of-range error strings directly to stay locale-independent.
        const std::string tapRangeMsg  = I18n::instance().get("error.tap_track_out_of_range");
        const std::string holdRangeMsg = I18n::instance().get("error.hold_track_out_of_range");
        ErrorNotifier::setSink([&hadOutOfRangeNotes, prevSink, tapRangeMsg, holdRangeMsg]
                               (ErrorNotifier::Level level, const std::string &msg) {
            if (msg.find(tapRangeMsg)  != std::string::npos ||
                msg.find(holdRangeMsg) != std::string::npos) {
                hadOutOfRangeNotes = true;
            }
            if (prevSink) prevSink(level, msg);
        });

        const bool chartOk_ =
            GameplayChartParser::parseChart(params.chartFilePath, params.keyCount, state->chartData);
        const bool sessionOk_ =
            state->gameplay.openChart(params.chartFilePath, params.keyCount);

        ErrorNotifier::setSink(prevSink);

        if (!chartOk_ || !sessionOk_) {
            MessageOverlay::push(MessageLevel::Error, tr("popup.error.chart_invalid_content"));
            // Fallback: render error and allow escape back
            auto errRoot = Renderer([state, tr] {
                return vbox({
                    text(tr("ui.gameplay.load_failed")) |
                        color(Color::RedLight) | bold | center,
                    text(state->params.chartFilePath) |
                        color(toColor(state->palette.textMuted)) | center,
                    text(tr("ui.gameplay.press_esc")) |
                        color(toColor(state->palette.textMuted)) | center,
                }) |
                bgcolor(toColor(state->palette.surfaceBg)) | flex;
            });
            return CatchEvent(errRoot, [state, onRoute = std::move(onRoute)](const Event &ev) {
                if (ev == Event::Escape || ev == Event::Character('q')) {
                    if (!state->routed) {
                        state->routed = true;
                        onRoute(UIScene::ChartList);
                    }
                    return true;
                }
                return false;
            });
        }
    }

    if (hadOutOfRangeNotes) {
        MessageOverlay::push(MessageLevel::Warning, tr("popup.warning.chart_notes_out_of_range"));
    }

    // Initialise lane pressed vector
    state->lanePressed.assign(static_cast<std::size_t>(params.keyCount), false);

    // Load and start audio
    state->audio.setVolume(RuntimeConfig::musicVolume);
    const bool audioOk = state->audio.loadSong(params.musicFilePath.c_str());
    if (audioOk) {
        state->gameplay.setChartClockDrivenByAudio(true);
        state->audio.playSong();
        state->audioStarted = true;
    } else {
        MessageOverlay::push(MessageLevel::Error, tr("popup.error.audio_load_failed"));
    }

    // Estimate total duration from chart end time (plus a small tail)
    {
        uint32_t endMs = state->chartData.getEndTimeMs();
        state->totalDurationMs = endMs > 0 ? endMs + 3000 : 60000;
    }

    // Background tick thread: posts Event::Custom at ~60fps
    state->bgRunning.store(true);
    state->bgThread = std::thread([st = state.get(), &screen]() {
        while (st->bgRunning.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            if (st->bgRunning.load(std::memory_order_relaxed)) {
                screen.PostEvent(Event::Custom);
            }
        }
    });

    // ── Key → lane map (pre-built for fast lookup) ──────────────────────────
    // Build reverse map: key code → lane index from RuntimeConfig::keyBindings
    std::unordered_map<uint8_t, int> keyToLane;
    {
        const auto &bindings = RuntimeConfig::keyBindings;
        for (int l = 0; l < static_cast<int>(params.keyCount) &&
                        l < static_cast<int>(bindings.size()); ++l) {
            const uint8_t kc = bindings[static_cast<std::size_t>(l)];
            if (kc != 0 && !keyToLane.contains(kc)) {
                keyToLane[kc] = l;
            }
        }
    }

    // ── Renderer ─────────────────────────────────────────────────────────────
    auto root = Renderer([state, tr] {
        using namespace ftxui;
        const GameplaySnapshot snap = state->gameplay.snapshot();

        const uint32_t displayTimeMs = snap.getCurrentChartTimeMs() +
                                        static_cast<uint32_t>(std::max(0, RuntimeConfig::chartDisplayOffsetMs));

        Element noteField = renderNoteField(
            state->chartData, displayTimeMs, state->params.keyCount,
            state->lanePressed, state->palette, snap);

        Element stats = renderStatsPanel(
            snap, state->palette, tr,
            state->totalDurationMs,
            RuntimeConfig::showEarlyLate,
            RuntimeConfig::showAPIndicator,
            RuntimeConfig::showFCIndicator);

        Element topBar = renderTopBar(
            state->params.songName,
            state->params.difficulty,
            state->params.keyCount,
            state->palette, tr);

        Element bottomBar = renderBottomBar(state->palette, tr);

        Element body = hbox({
            noteField | size(WIDTH, EQUAL, state->params.keyCount * 5 + 2),
            text("  "),
            stats,
        }) | flex;

        return vbox({
                   topBar,
                   separator(),
                   body,
                   separator(),
                   bottomBar,
               }) |
               bgcolor(toColor(state->palette.surfaceBg)) |
               color(toColor(state->palette.textPrimary)) |
               flex;
    });

    // ── Event handler ─────────────────────────────────────────────────────────
    auto app = CatchEvent(
        root,
        [state, keyToLane = std::move(keyToLane),
         onRoute = std::move(onRoute), tr](const Event &event) mutable {

            // ── Tick: advance game clock & check key timeouts ──────────────
            if (event == Event::Custom) {
                if (!state->resultTriggered) {
                    // Advance clock via audio position
                    if (state->audioStarted) {
                        if (state->audio.isFinished()) {
                            state->gameplay.setAudioFinished(true);
                        }
                        state->gameplay.advanceAudioTimeMs(state->audio.positionMs());
                    } else {
                        // No audio: advance by wall-clock
                        state->gameplay.advanceChartTimeMs(
                            state->gameplay.clockSnapshot().getChartTimeMs() + 16);
                    }

                    // Auto key-up for keys that haven't repeated recently
                    const auto now = std::chrono::steady_clock::now();
                    std::vector<uint8_t> toRelease;
                    for (const auto &[kc, ts] : state->heldKeys) {
                        const auto elapsed =
                            std::chrono::duration_cast<std::chrono::milliseconds>(now - ts).count();
                        if (elapsed > kKeyHoldTimeoutMs) {
                            toRelease.push_back(kc);
                        }
                    }
                    for (const uint8_t kc : toRelease) {
                        state->gameplay.onKeyUp(kc);
                        state->heldKeys.erase(kc);
                        // Clear lane visual
                        const auto it = keyToLane.find(kc);
                        if (it != keyToLane.end() &&
                            it->second < static_cast<int>(state->lanePressed.size())) {
                            state->lanePressed[static_cast<std::size_t>(it->second)] = false;
                        }
                    }

                    // Check result ready
                    if (state->gameplay.isResultReady() && !state->routed) {
                        state->resultTriggered = true;
                        state->routed = true;
                        state->bgRunning.store(false);

                        // Populate settlement params
                        const GameplayFinalResult res = state->gameplay.finalResult();
                        SettlementRouteParams sp;
                        sp.perfectCount   = res.getPerfectCount();
                        sp.greatCount     = res.getGreatCount();
                        sp.missCount      = res.getMissCount();
                        sp.earlyCount     = res.getEarlyCount();
                        sp.lateCount      = res.getLateCount();
                        sp.score          = res.getScore();
                        sp.accuracy       = res.getAccuracy();
                        sp.maxCombo       = res.getMaxCombo();
                        sp.chartNoteCount = res.getChartNoteCount();
                        sp.chartMaxScore  = res.getChartMaxScore();
                        sp.songName       = state->params.songName;
                        sp.chartID        = state->params.chartID;
                        sp.difficulty     = state->params.difficulty;

                        // Save record via settlement instance
                        GameplaySettlement settlement;
                        sp.saveSucceeded  = settlement.onEnterSettlement(
                            state->gameplay,
                            state->params.chartID,
                            state->params.songName);

                        // Push result popup
                        if (sp.saveSucceeded) {
                            MessageOverlay::push(MessageLevel::Info,
                                I18n::instance().get("popup.info.record_saved"));
                        } else if (!UserContext::hasLoggedInUser() || UserContext::isGuestUser()) {
                            MessageOverlay::push(MessageLevel::Error,
                                I18n::instance().get("popup.error.record_save_no_login"));
                        } else {
                            MessageOverlay::push(MessageLevel::Error,
                                I18n::instance().get("popup.error.record_save_failed"));
                        }

                        UIBus::pendingSettlement = sp;
                        onRoute(UIScene::GameplaySettlement);
                    }
                }
                return true;
            }

            // ── Escape: quit back to chart list ───────────────────────────
            if (event == Event::Escape) {
                if (!state->routed) {
                    state->routed = true;
                    state->bgRunning.store(false);
                    state->audio.stopSong();
                    onRoute(UIScene::ChartList);
                }
                return true;
            }

            // ── Key press ─────────────────────────────────────────────────
            if (!event.is_character()) return false;
            const std::string &ch = event.character();
            if (ch.empty() || ch.size() > 1) return false;

            const auto kc = static_cast<uint8_t>(ch[0]);
            if (!keyToLane.contains(kc)) return false;

            const int lane = keyToLane.at(kc);
            if (lane < static_cast<int>(state->lanePressed.size())) {
                state->lanePressed[static_cast<std::size_t>(lane)] = true;
            }

            if (!state->heldKeys.contains(kc)) {
                state->gameplay.onKeyDown(kc);
            }
            state->heldKeys[kc] = std::chrono::steady_clock::now();
            return true;
        });

    return app;
}

} // namespace ui
