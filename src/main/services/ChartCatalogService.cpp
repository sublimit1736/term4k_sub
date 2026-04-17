#include "ChartCatalogService.h"

#include "ChartIOService.h"
#include "I18nService.h"
#include "dao/ProofedRecordsDAO.h"
#include "entities/Chart.h"
#include "utils/ErrorNotifier.h"
#include "utils/RatingUtils.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>

namespace fs = std::filesystem;

namespace {

enum class PlayableEventType : uint8_t { Tap, HoldHead, HoldTail, };

struct LaneTimeKey {
    uint8_t lane = 0;
    uint32_t timeMs = 0;

    bool operator<(const LaneTimeKey &other) const {
        if (timeMs != other.timeMs) return timeMs < other.timeMs;
        return lane < other.lane;
    }
};

uint8_t hexVal(const char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0;
}

uint8_t parseHexField2(const std::string &str, const int start) {
    uint8_t val = 0;
    for (int i = start; i < start + 2; ++i) {
        val = static_cast<uint8_t>((val << 4) | hexVal(str[i]));
    }
    return val;
}

uint32_t parseHexField8(const std::string &str, const int start) {
    uint32_t val = 0;
    for (int i = start; i < start + 8; ++i) {
        val = hexVal(str[i]) | (val << 4);
    }
    return val;
}

std::string playableEventTypeName(const PlayableEventType type) {
    switch (type) {
        case PlayableEventType::Tap:
            return "tap";
        case PlayableEventType::HoldHead:
            return "hold_head";
        case PlayableEventType::HoldTail:
            return "hold_tail";
    }
    return "unknown";
}

struct ConflictResolution {
    std::optional<PlayableEventType> keptType;
    std::string resolution;
};

ConflictResolution resolveConflict(const PlayableEventType existing,
                                   const PlayableEventType incoming) {
    if (existing == incoming) {
        return {existing, "merge_same_type"};
    }
    if ((existing == PlayableEventType::Tap && incoming == PlayableEventType::HoldHead) ||
        (existing == PlayableEventType::HoldHead && incoming == PlayableEventType::Tap)) {
        return {PlayableEventType::Tap, "keep_tap_drop_hold_head"};
    }
    if ((existing == PlayableEventType::Tap && incoming == PlayableEventType::HoldTail) ||
        (existing == PlayableEventType::HoldTail && incoming == PlayableEventType::Tap)) {
        return {PlayableEventType::HoldTail, "keep_hold_tail_drop_tap"};
    }
    if ((existing == PlayableEventType::HoldHead && incoming == PlayableEventType::HoldTail) ||
        (existing == PlayableEventType::HoldTail && incoming == PlayableEventType::HoldHead)) {
        return {std::nullopt, "drop_hold_head_and_hold_tail"};
    }
    return {existing, "keep_existing"};
}

void appendOrResolvePlayableEvent(std::map<LaneTimeKey, PlayableEventType> &events,
                                  std::vector<PlayableNoteConflict> &conflicts,
                                  const uint8_t lane,
                                  const uint32_t timeMs,
                                  const PlayableEventType incomingType) {
    const LaneTimeKey key{lane, timeMs};
    const auto existingIt = events.find(key);
    if (existingIt == events.end()) {
        events[key] = incomingType;
        return;
    }

    const PlayableEventType existingType = existingIt->second;
    const ConflictResolution resolved = resolveConflict(existingType, incomingType);

    conflicts.push_back(PlayableNoteConflict{
        lane,
        timeMs,
        playableEventTypeName(existingType),
        playableEventTypeName(incomingType),
        resolved.resolution,
    });

    if (!resolved.keptType.has_value()) {
        events.erase(existingIt);
        return;
    }
    existingIt->second = resolved.keptType.value();
}

std::string issueI18nKey(const ChartDetectionIssue issue) {
    switch (issue) {
        case ChartDetectionIssue::MissingMeta:
            return "chartdetect.issue.missing_meta";
        case ChartDetectionIssue::MissingChart:
            return "chartdetect.issue.missing_chart";
        case ChartDetectionIssue::MissingMusic:
            return "chartdetect.issue.missing_music";
        case ChartDetectionIssue::MetadataOpenFailed:
            return "chartdetect.issue.meta_open_failed";
        case ChartDetectionIssue::MissingID:
            return "chartdetect.issue.missing_id";
        case ChartDetectionIssue::FolderIDMismatch:
            return "chartdetect.issue.folder_id_mismatch";
    }
}

ChartDetectionFailure makeFailure(const fs::path &folder,
                                  const ChartDetectionIssue issue,
                                  std::string metadataID = "") {
    return {folder.string(), issue, std::move(metadataID), I18nService::instance().get(issueI18nKey(issue))};
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool tryBuildChartItem(const fs::path &folder,
                       ChartCatalogEntry &out,
                       ChartDetectionFailure *failure) {
    const fs::path metadataPath = folder / "meta.json";
    const fs::path chartPath = folder / "chart.t4k";
    const fs::path musicPath = folder / "music.ogg";

    // Missing any required fixed file means this folder is not a valid chart package.
    if (!fs::exists(metadataPath) || !fs::is_regular_file(metadataPath)) {
        if (failure != nullptr) *failure = makeFailure(folder, ChartDetectionIssue::MissingMeta);
        return false;
    }
    if (!fs::exists(chartPath) || !fs::is_regular_file(chartPath)) {
        if (failure != nullptr) *failure = makeFailure(folder, ChartDetectionIssue::MissingChart);
        return false;
    }
    if (!fs::exists(musicPath) || !fs::is_regular_file(musicPath)) {
        if (failure != nullptr) *failure = makeFailure(folder, ChartDetectionIssue::MissingMusic);
        return false;
    }

    std::ifstream in(metadataPath);
    if (!in.is_open()) {
        ErrorNotifier::notify("ChartCatalogService::tryBuildChartItem",
                              I18nService::instance().get("error.metadata_file_open_failed") + ": " +
                                  metadataPath.string());
        if (failure != nullptr) {
            *failure = makeFailure(folder, ChartDetectionIssue::MetadataOpenFailed);
        }
        return false;
    }

    std::ostringstream ss;
    ss << in.rdbuf();

    Chart chart = Chart::deserializeString(ss.str());
    // New rule: folder name must match metadata id; mismatches are ignored.
    if (chart.getID().empty()) {
        if (failure != nullptr) *failure = makeFailure(folder, ChartDetectionIssue::MissingID);
        return false;
    }
    if (chart.getID() != folder.filename().string()) {
        if (failure != nullptr) {
            *failure = makeFailure(folder, ChartDetectionIssue::FolderIDMismatch, chart.getID());
        }
        return false;
    }
    if (chart.getDisplayName().empty()) {
        chart.setDisplayName(folder.filename().string());
    }

    out.chart = std::move(chart);
    out.folderPath = folder.string();
    out.metadataFilePath = metadataPath.string();
    out.chartFilePath = chartPath.string();
    out.musicFilePath = musicPath.string();
    return true;
}

bool isUIDRecord(const std::vector<std::string> &fields) {
    if (fields.size() < 7) return false;
    return string_utils::isDigitsOnly(fields[0]);
}

double singleChartEvaluation(const float difficulty, const float accuracy) {
    return rating_utils::singleChartEvaluation(difficulty, accuracy);
}

// Aggregates per-chart play stats for a user from verified records.
// Looks up chart difficulty from the already-built catalog so the directory
// does not need to be scanned a second time.
std::map<std::string, ChartPlayStats> aggregateStatsFromCatalog(
        const ChartCatalogMap &catalog,
        const std::string &uid) {
    enum class AchievementTier : uint8_t { None = 0, FC = 1, AP = 2, ULT = 3 };

    auto currentTier = [](const ChartPlayStats &stats) {
        if (stats.hasULT) return AchievementTier::ULT;
        if (stats.hasAP) return AchievementTier::AP;
        if (stats.hasFC) return AchievementTier::FC;
        return AchievementTier::None;
    };

    auto applyTier = [](ChartPlayStats &stats, const AchievementTier tier) {
        stats.hasFC = false;
        stats.hasAP = false;
        stats.hasULT = false;
        if (tier == AchievementTier::FC) stats.hasFC = true;
        if (tier == AchievementTier::AP) stats.hasAP = true;
        if (tier == AchievementTier::ULT) stats.hasULT = true;
    };

    std::map<std::string, ChartPlayStats> statsByChart;
    if (uid.empty()) return statsByChart;

    const auto records = ProofedRecordsDAO::readVerifiedRecordByUID(uid);
    for (const auto &record : records) {
        std::istringstream iss(record);
        std::vector<std::string> fields;
        std::string token;
        while (iss >> token) {
            fields.push_back(token);
        }
        if (fields.size() < 6) continue;

        const bool uidFormat = isUIDRecord(fields);
        const std::size_t chartIdx    = uidFormat ? 1 : 0;
        const std::size_t scoreIdx    = uidFormat ? 4 : 3;
        const std::size_t accuracyIdx = uidFormat ? 5 : 4;
        const std::size_t maxComboIdx = uidFormat ? 7 : 6;
        const std::size_t noteCountIdx= uidFormat ? 8 : 7;
        const std::size_t perfectIdx  = uidFormat ? 9 : 8;
        const std::size_t earlyIdx    = uidFormat ? 10 : 9;
        const std::size_t lateIdx     = uidFormat ? 11 : 10;

        uint32_t score = 0;
        float accuracy = 0.0f;
        try {
            score    = static_cast<uint32_t>(std::stoul(fields.at(scoreIdx)));
            accuracy = std::stof(fields.at(accuracyIdx));
        } catch (...) {
            continue;
        }

        const std::string &chartID = fields.at(chartIdx);
        auto &stats = statsByChart[chartID];
        ++stats.playCount;
        if (score    > stats.bestScore)    stats.bestScore    = score;
        if (accuracy > stats.bestAccuracy) stats.bestAccuracy = accuracy;

        // Use already-built catalog for difficulty lookup (no extra directory scan).
        const auto catalogIt = catalog.find(chartID);
        if (catalogIt != catalog.end()) {
            stats.bestSingleRating = std::max(
                stats.bestSingleRating,
                singleChartEvaluation(catalogIt->second.chart.getDifficulty(), accuracy));
        }

        bool hasNoteCount = false;
        uint32_t noteCount = 0;
        if (fields.size() > noteCountIdx) {
            try {
                noteCount = static_cast<uint32_t>(std::stoul(fields.at(noteCountIdx)));
                hasNoteCount = (noteCount > 0);
            } catch (...) { /* best-effort parse; skip record field on failure */ }
        }

        bool recordFC = false;
        if (hasNoteCount && fields.size() > maxComboIdx) {
            try {
                const auto maxCombo = static_cast<uint32_t>(std::stoul(fields.at(maxComboIdx)));
                recordFC = (maxCombo == noteCount);
            } catch (...) { /* best-effort parse; skip record field on failure */ }
        }

        bool recordAP = false;
        if (hasNoteCount && fields.size() > perfectIdx) {
            try {
                const auto perfectCount = static_cast<uint32_t>(std::stoul(fields.at(perfectIdx)));
                recordAP = (perfectCount == noteCount);
            } catch (...) { /* best-effort parse; skip record field on failure */ }
        }

        bool recordULT = false;
        if (fields.size() > lateIdx && fields.size() > earlyIdx) {
            try {
                const auto early = static_cast<uint32_t>(std::stoul(fields.at(earlyIdx)));
                const auto late  = static_cast<uint32_t>(std::stoul(fields.at(lateIdx)));
                recordULT = (early == 0 && late == 0);
            } catch (...) { /* best-effort parse; skip record field on failure */ }
        }

        AchievementTier recordTier = AchievementTier::None;
        if (recordULT) recordTier = AchievementTier::ULT;
        else if (recordAP) recordTier = AchievementTier::AP;
        else if (recordFC) recordTier = AchievementTier::FC;

        if (recordTier > currentTier(stats)) {
            applyTier(stats, recordTier);
        }
    }

    return statsByChart;
}

} // namespace

std::vector<PlayableNoteConflict> ChartCatalogService::checkChartCompliance(const std::string &chartFilePath,
                                                                            const uint16_t keyCount) {
    std::vector<PlayableNoteConflict> conflicts;
    if (chartFilePath.empty()) return conflicts;

    std::ifstream chartFile(chartFilePath, std::ios::in);
    if (!chartFile.is_open()) {
        ErrorNotifier::notify("ChartCatalogService::checkChartCompliance",
                              I18nService::instance().get("error.chart_open_failed") + ": " + chartFilePath);
        return conflicts;
    }

    std::string beginMarker;
    std::getline(chartFile, beginMarker);
    if (beginMarker != "t4kcb") {
        ErrorNotifier::notify("ChartCatalogService::checkChartCompliance",
                              I18nService::instance().get("error.chart_invalid_format") + ": " + chartFilePath);
        return conflicts;
    }

    std::map<LaneTimeKey, PlayableEventType> events;
    std::string line;
    while (std::getline(chartFile, line)) {
        if (line == "t4kce") break;
        if (line.size() < 2) continue;

        const uint8_t type = parseHexField2(line, 0);
        if (type == tap) {
            if (line.size() < 12) continue;
            const uint8_t lane = parseHexField2(line, 2);
            if (lane >= keyCount) continue;
            const uint32_t t = parseHexField8(line, 4);
            appendOrResolvePlayableEvent(events, conflicts, lane, t, PlayableEventType::Tap);
        } else if (type == hold) {
            if (line.size() < 20) continue;
            const uint8_t lane = parseHexField2(line, 2);
            if (lane >= keyCount) continue;
            const uint32_t t1 = parseHexField8(line, 4);
            const uint32_t t2 = parseHexField8(line, 12);
            appendOrResolvePlayableEvent(events, conflicts, lane, t1, PlayableEventType::HoldHead);
            appendOrResolvePlayableEvent(events, conflicts, lane, t2, PlayableEventType::HoldTail);
        }
    }

    for (const auto &conflict : conflicts) {
        ErrorNotifier::notifyWarning(
            "ChartCatalogService::checkChartCompliance",
            I18nService::instance().get("warning.chart_compliance_conflict") +
                " (lane=" + std::to_string(static_cast<unsigned int>(conflict.lane)) +
                ", timeMs=" + std::to_string(conflict.timeMs) +
                ", existingType=" + conflict.existingType +
                ", incomingType=" + conflict.incomingType +
                ", resolution=" + conflict.resolution + ")");
    }

    return conflicts;
}

ChartCatalogMap ChartCatalogService::loadCatalogForUID(const std::string &chartsRoot,
                                                       const std::string &uid,
                                                       std::vector<ChartDetectionFailure> *failures) {
    if (failures != nullptr) failures->clear();

    ChartCatalogMap items;
    const fs::path root(chartsRoot);

    if (!fs::exists(root) || !fs::is_directory(root)) {
        ErrorNotifier::notify("ChartCatalogService::loadCatalogForUID",
                              I18nService::instance().get("error.charts_root_not_directory") + ": " + chartsRoot);
        return items;
    }

    // Single directory scan: build all catalog entries first.
    for (const auto &entry : fs::directory_iterator(root)) {
        if (!entry.is_directory()) continue;

        ChartCatalogEntry item;
        ChartDetectionFailure failure;
        if (!tryBuildChartItem(entry.path(), item, failures != nullptr ? &failure : nullptr)) {
            if (failures != nullptr) failures->push_back(std::move(failure));
            continue;
        }

        items[item.chart.getID()] = std::move(item);
    }

    // Aggregate stats using the already-built catalog for difficulty lookups
    // (avoids the previous second directory scan inside buildStatsByChart).
    if (!uid.empty()) {
        const auto statsByChart = aggregateStatsFromCatalog(items, uid);
        for (auto &[id, entry] : items) {
            const auto it = statsByChart.find(id);
            if (it != statsByChart.end()) entry.stats = it->second;
        }
    }

    return items;
}

ChartCatalogMap ChartCatalogService::loadCatalogForUID(const std::string &chartsRoot,
                                                       const std::string &uid) {
    return loadCatalogForUID(chartsRoot, uid, nullptr);
}

std::vector<std::string> ChartCatalogService::sortCatalogKeys(const ChartCatalogMap &items,
                                                              const ChartListSortKey key,
                                                              const SortOrder order) {
    std::vector<std::string> keys;
    keys.reserve(items.size());
    for (const auto &[id, _] : items) {
        keys.push_back(id);
    }

    const bool asc = (order == SortOrder::Ascending);

    if (key == ChartListSortKey::DisplayName) {
        // Precompute lowercase names once to avoid repeated allocation per comparison.
        std::map<std::string, std::string> lowerNames;
        for (const auto &k : keys) {
            lowerNames[k] = toLower(items.at(k).chart.getDisplayName());
        }
        std::ranges::stable_sort(keys, [&](const std::string &a, const std::string &b) {
            return asc ? (lowerNames[a] < lowerNames[b]) : (lowerNames[a] > lowerNames[b]);
        });
        return keys;
    }

    std::ranges::stable_sort(keys, [&](const std::string &lhsID, const std::string &rhsID) {
        const auto &a = items.at(lhsID);
        const auto &b = items.at(rhsID);
        switch (key) {
            case ChartListSortKey::Difficulty:
                return asc ? (a.chart.getDifficulty() < b.chart.getDifficulty())
                           : (a.chart.getDifficulty() > b.chart.getDifficulty());
            case ChartListSortKey::BestAccuracy:
                return asc ? (a.stats.bestAccuracy < b.stats.bestAccuracy)
                           : (a.stats.bestAccuracy > b.stats.bestAccuracy);
            default: return false;
        }
    });
    return keys;
}
