#pragma once

#include "entities/Chart.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Sort keys for chart list ordering.
enum class ChartListSortKey {
    DisplayName,
    // Sort by chart display name (lexicographical).
    Difficulty,
    // Sort by chart difficulty.
    BestAccuracy,
    // Sort by player's best accuracy (0 when no record exists).
};

// Sort direction.
enum class SortOrder {
    Ascending,
    // Ascending order.
    Descending,
    // Descending order.
};

// Detection issue types produced during chart directory scanning.
enum class ChartDetectionIssue {
    MissingMeta,
    // Missing meta.json.
    MissingChart,
    // Missing chart.t4k.
    MissingMusic,
    // Missing music.ogg.
    MetadataOpenFailed,
    // Metadata file cannot be opened.
    MissingID,
    // Metadata is missing id.
    FolderIDMismatch,
    // Folder name does not match metadata id.
};

// Per-chart play stats for a user (based on verified records).
struct ChartPlayStats {
    uint32_t playCount = 0;    // Number of plays.
    uint32_t bestScore = 0;    // Best score.
    float bestAccuracy = 0.0f; // Best accuracy.
};

// Detailed failure information for a chart folder scan.
struct ChartDetectionFailure {
    std::string folderPath; // Failed folder path.
    ChartDetectionIssue issue = ChartDetectionIssue::MissingMeta;
    std::string metadataID;     // Filled only for some issue types.
    std::string localizedIssue; // Localized failure text.
};

// Combined chart catalog entry with metadata and stats.
struct ChartCatalogEntry {
    Chart chart;                  // Parsed chart metadata.
    std::string folderPath;       // Chart folder path.
    std::string metadataFilePath; // meta.json path.
    std::string chartFilePath;    // chart.t4k path.
    std::string musicFilePath;    // music.ogg path.
    ChartPlayStats stats;         // User stats on this chart.
};

// Structured business-layer representation for one play record.
struct ChartRecordEntry {
    std::string uid;           // User UID.
    std::string chartID;       // Chart ID.
    Chart chart;               // Linked chart (fallback when metadata is missing).
    uint32_t score     = 0;    // Score for this record.
    float accuracy     = 0.0f; // Accuracy for this record.
    uint32_t timestamp = 0;    // Timestamp for this record.
};

// Chart catalog map keyed by chart ID.
using ChartCatalogMap = std::map<std::string, ChartCatalogEntry>;

// Record map keyed by unique record key.
using ChartRecordMap = std::map<std::string, ChartRecordEntry>;

// Record collection: order for display sequence, records for entity storage.
struct ChartRecordCollection {
    ChartRecordMap records;
    std::vector<std::string> order;
};

// Service for chart directory scanning and user stat aggregation.
class ChartCatalogService {
public:
    // Scans chartsRoot and aggregates best stats for the given uid.
    // When failures is non-null, ignored folders are reported there.
    static ChartCatalogMap loadCatalogForUID(const std::string &chartsRoot,
                                             const std::string &uid,
                                             std::vector<ChartDetectionFailure>* failures
        );

    // Same as above but does not collect scan failures.
    static ChartCatalogMap loadCatalogForUID(const std::string &chartsRoot,
                                             const std::string &uid
        );

    // Returns sorted key list for UI traversal without copying full entries.
    static std::vector<std::string> sortCatalogKeys(const ChartCatalogMap &items,
                                                    ChartListSortKey key,
                                                    SortOrder order
        );
};

