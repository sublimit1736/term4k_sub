#include "GameplayChartService.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "services/ChartCatalogService.h"
#include "services/ChartIOService.h"
#include "services/I18nService.h"
#include "utils/ErrorNotifier.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace {
    enum class PlayableType { Tap, HoldHead, HoldTail, };

    enum class ConflictAction { KeepExisting, KeepIncoming, DropBoth, };

    struct LaneTimeKey {
        uint8_t lane    = 0;
        uint32_t timeMs = 0;

        bool operator<(const LaneTimeKey &other) const {
            if (timeMs != other.timeMs) return timeMs < other.timeMs;
            return lane < other.lane;
        }
    };

    struct RawTapNote {
        uint8_t lane    = 0;
        uint32_t timeMs = 0;
        bool kept       = true;
    };

    struct RawHoldNote {
        uint8_t lane        = 0;
        uint32_t headTimeMs = 0;
        uint32_t tailTimeMs = 0;
        bool headKept       = true;
        bool tailKept       = true;
    };

    struct EventRef {
        PlayableType type     = PlayableType::Tap;
        std::size_t noteIndex = 0;
    };

    ConflictAction resolveConflict(const PlayableType existing, const PlayableType incoming) {
        if (existing == incoming) return ConflictAction::KeepExisting;

        if ((existing == PlayableType::Tap && incoming == PlayableType::HoldHead) ||
            (existing == PlayableType::HoldHead && incoming == PlayableType::Tap)){
            return existing == PlayableType::Tap ? ConflictAction::KeepExisting : ConflictAction::KeepIncoming;
        }

        if ((existing == PlayableType::Tap && incoming == PlayableType::HoldTail) ||
            (existing == PlayableType::HoldTail && incoming == PlayableType::Tap)){
            return existing == PlayableType::HoldTail ? ConflictAction::KeepExisting : ConflictAction::KeepIncoming;
        }

        if ((existing == PlayableType::HoldHead && incoming == PlayableType::HoldTail) ||
            (existing == PlayableType::HoldTail && incoming == PlayableType::HoldHead)){
            return ConflictAction::DropBoth;
        }

        return ConflictAction::KeepExisting;
    }

    uint8_t hexVal(const char c) {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        return 0;
    }

    uint8_t parseHexField2(const std::string &str, const int start) {
        uint8_t val = 0;
        for (int i = start; i < start + 2; ++i){
            val = static_cast<uint8_t>((val << 4) | hexVal(str[i]));
        }
        return val;
    }

    uint32_t parseHexField8(const std::string &str, const int start) {
        uint32_t val = 0;
        for (int i = start; i < start + 8; ++i){
            val = hexVal(str[i]) | (val << 4);
        }
        return val;
    }

    uint32_t applyChartOffset(const uint32_t rawTimeMs) {
        const int64_t shifted = static_cast<int64_t>(rawTimeMs) + static_cast<int64_t>(RuntimeConfigs::chartOffsetMs);
        if (shifted <= 0) return 0;
        if (shifted > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())){
            return std::numeric_limits<uint32_t>::max();
        }
        return static_cast<uint32_t>(shifted);
    }

    std::string resolveChartPath(const std::string &chartFilePath) {
        if (chartFilePath.empty()) return "";

        const std::filesystem::path path(chartFilePath);
        if (path.is_absolute()) return chartFilePath;

        AppDirs::init();
        return AppDirs::chartsDir() + chartFilePath;
    }

    uint64_t calculateMaxScore(const uint32_t noteCount) {
        const auto n = static_cast<uint64_t>(noteCount);
        return (n * n + 5ULL * n) / 2ULL;
    }

    void markDropped(const EventRef &ref,
                     std::vector<RawTapNote> &rawTaps,
                     std::vector<RawHoldNote> &rawHolds
        ) {
        if (ref.type == PlayableType::Tap){
            if (ref.noteIndex < rawTaps.size()) rawTaps[ref.noteIndex].kept = false;
            return;
        }

        if (ref.noteIndex >= rawHolds.size()) return;
        if (ref.type == PlayableType::HoldHead){
            rawHolds[ref.noteIndex].headKept = false;
        }
        else if (ref.type == PlayableType::HoldTail){
            rawHolds[ref.noteIndex].tailKept = false;
        }
    }

    void upsertPlayableEvent(const uint8_t lane,
                             const uint32_t timeMs,
                             const EventRef incoming,
                             std::map<LaneTimeKey, EventRef> &events,
                             std::vector<RawTapNote> &rawTaps,
                             std::vector<RawHoldNote> &rawHolds
        ) {
        const LaneTimeKey key{lane, timeMs};
        const auto it = events.find(key);
        if (it == events.end()){
            events[key] = incoming;
            return;
        }

        const EventRef existing     = it->second;
        const ConflictAction action = resolveConflict(existing.type, incoming.type);
        if (action == ConflictAction::KeepExisting){
            markDropped(incoming, rawTaps, rawHolds);
            return;
        }
        if (action == ConflictAction::KeepIncoming){
            markDropped(existing, rawTaps, rawHolds);
            it->second = incoming;
            return;
        }

        markDropped(existing, rawTaps, rawHolds);
        markDropped(incoming, rawTaps, rawHolds);
        events.erase(it);
    }
} // namespace

bool GameplayChartService::parseChart(const std::string &chartFilePath,
                                      const uint16_t keyCount,
                                      GameplayChartData &outChart
    ) {
    outChart = GameplayChartData{};
    if (keyCount == 0) return false;

    const std::string resolvedPath = resolveChartPath(chartFilePath);
    if (resolvedPath.empty()) return false;

    ChartCatalogService::checkChartCompliance(resolvedPath, keyCount);

    std::ifstream chartFile(resolvedPath, std::ios::in);
    if (!chartFile.is_open()){
        ErrorNotifier::notify("GameplayInstance::openChart",
                              I18nService::instance().get("error.chart_open_failed") + ": " + resolvedPath);
        return false;
    }

    std::string beginMarker;
    std::getline(chartFile, beginMarker);
    if (beginMarker != "t4kcb"){
        ErrorNotifier::notify("GameplayInstance::openChart",
                              I18nService::instance().get("error.chart_invalid_format") + ": " + resolvedPath);
        return false;
    }

    std::vector<RawTapNote> rawTaps;
    std::vector<RawHoldNote> rawHolds;
    std::map<LaneTimeKey, EventRef> events;

    std::string line;
    while (std::getline(chartFile, line)){
        if (line == "t4kce") break;
        if (line.size() < 2) continue;

        const uint8_t type = parseHexField2(line, 0);
        if (type == tap){
            if (line.size() < 12) continue;
            const uint8_t lane = parseHexField2(line, 2);
            if (lane >= keyCount) continue;
            const uint32_t timeMs = applyChartOffset(parseHexField8(line, 4));

            const std::size_t idx = rawTaps.size();
            rawTaps.push_back(RawTapNote{lane, timeMs, true});
            upsertPlayableEvent(lane, timeMs, EventRef{PlayableType::Tap, idx}, events, rawTaps, rawHolds);
        }
        else if (type == hold){
            if (line.size() < 20) continue;
            const uint8_t lane = parseHexField2(line, 2);
            if (lane >= keyCount) continue;

            const uint32_t headTimeMs = applyChartOffset(parseHexField8(line, 4));
            const uint32_t tailTimeMs = applyChartOffset(parseHexField8(line, 12));

            const std::size_t idx = rawHolds.size();
            rawHolds.push_back(RawHoldNote{lane, headTimeMs, tailTimeMs, true, true});
            upsertPlayableEvent(lane, headTimeMs, EventRef{PlayableType::HoldHead, idx}, events, rawTaps, rawHolds);
            upsertPlayableEvent(lane, tailTimeMs, EventRef{PlayableType::HoldTail, idx}, events, rawTaps, rawHolds);
        }
    }

    for (const auto &tapNote: rawTaps){
        if (!tapNote.kept) continue;
        outChart.mutableTaps().emplace_back(tapNote.lane, tapNote.timeMs);
        outChart.setEndTimeMs(std::max(outChart.getEndTimeMs(), tapNote.timeMs));
    }

    for (const auto &holdNote: rawHolds){
        if (!holdNote.headKept || !holdNote.tailKept) continue;
        if (holdNote.tailTimeMs < holdNote.headTimeMs) continue;
        outChart.mutableHolds().emplace_back(holdNote.lane, holdNote.headTimeMs, holdNote.tailTimeMs);
        outChart.setEndTimeMs(std::max(outChart.getEndTimeMs(), holdNote.tailTimeMs));
    }

    outChart.setNoteCount(static_cast<uint32_t>(outChart.getTaps().size() + outChart.getHolds().size()));
    outChart.setMaxScore(calculateMaxScore(outChart.getNoteCount()));
    return true;
}