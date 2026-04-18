#include "ChartListInstance.h"

#include "services/ChartCatalogService.h"

#include <cctype>
#include <filesystem>

namespace {
    std::string trim(std::string value) {
        std::size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0){
            ++begin;
        }

        std::size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0){
            --end;
        }
        return value.substr(begin, end - begin);
    }
}

bool ChartListInstance::refresh(const std::string &chartsRoot, const std::string &uid) {
    if (!std::filesystem::exists(chartsRoot) || !std::filesystem::is_directory(chartsRoot)){
        chartItems.clear();
        orderedChartIds.clear();
        filteredOrderedChartIds.clear();
        displayNameTrie.clear();
        artistTrie.clear();
        charterTrie.clear();
        resetSearchCursors();
        detectionFailures.clear();
        return false;
    }

    chartItems      = ChartCatalogService::loadCatalogForUID(chartsRoot, uid, &detectionFailures);
    orderedChartIds =
        ChartCatalogService::sortCatalogKeys(chartItems, ChartListSortKey::DisplayName, SortOrder::Ascending);
    rebuildSearchIndex();
    applySearchFilter();

    return true;
}

void ChartListInstance::sort(ChartListSortKey key, SortOrder order) {
    orderedChartIds = ChartCatalogService::sortCatalogKeys(chartItems, key, order);
    rebuildSearchIndex();
    applySearchFilter();
}

const ChartCatalogMap &ChartListInstance::items() const {
    return chartItems;
}

const std::vector<std::string> &ChartListInstance::orderedChartIDs() const {
    return orderedChartIds;
}

const std::vector<std::string> &ChartListInstance::filteredOrderedChartIDs() const {
    return filteredOrderedChartIds;
}

void ChartListInstance::setSearchQuery(const std::string &query) {
    currentSearchQuery = trim(query);
    applySearchFilter();
}

void ChartListInstance::setSearchMode(ChartSearchMode mode) {
    if (currentSearchMode == mode) return;
    currentSearchMode = mode;
    applySearchFilter();
}

ChartSearchMode ChartListInstance::searchMode() const {
    return currentSearchMode;
}

void ChartListInstance::clearSearch() {
    currentSearchQuery.clear();
    applySearchFilter();
}

const std::string &ChartListInstance::searchQuery() const {
    return currentSearchQuery;
}

const std::vector<ChartDetectionFailure> &ChartListInstance::failures() const {
    return detectionFailures;
}

void ChartListInstance::rebuildSearchIndex() {
    displayNameTrie.clear();
    artistTrie.clear();
    charterTrie.clear();
    resetSearchCursors();

    for (std::size_t i = 0; i < orderedChartIds.size(); ++i){
        const auto &id = orderedChartIds[i];
        const auto it  = chartItems.find(id);
        if (it == chartItems.end()) continue;

        const auto &chart = it->second.chart;
        if (!chart.getDisplayName().empty()) displayNameTrie.insert(chart.getDisplayName(), i);
        if (!chart.getArtist().empty()) artistTrie.insert(chart.getArtist(), i);
        if (!chart.getCharter().empty()) charterTrie.insert(chart.getCharter(), i);
    }
}

void ChartListInstance::resetSearchCursors() {
    displayNameCursor = PrefixTrie::SearchCursor{};
    artistCursor      = PrefixTrie::SearchCursor{};
    charterCursor     = PrefixTrie::SearchCursor{};
}

void ChartListInstance::applySearchFilter() {
    if (currentSearchQuery.empty()){
        filteredOrderedChartIds = orderedChartIds;
        return;
    }

    std::vector<std::size_t> matchedIndices;
    switch (currentSearchMode){
        // Trie cursor supports incremental search and reuses previous state across typing.
        case ChartSearchMode::DisplayName: matchedIndices = displayNameTrie.
                                           searchByPrefixIncremental(currentSearchQuery, displayNameCursor);
            break;
        case ChartSearchMode::Artist: matchedIndices = artistTrie.searchByPrefixIncremental(currentSearchQuery,
                                               artistCursor);
            break;
        case ChartSearchMode::Charter: matchedIndices = charterTrie.searchByPrefixIncremental(currentSearchQuery,
                                                charterCursor);
            break;
    }

    filteredOrderedChartIds.clear();
    filteredOrderedChartIds.reserve(matchedIndices.size());
    for (const auto i: matchedIndices){
        if (i < orderedChartIds.size()){
            filteredOrderedChartIds.push_back(orderedChartIds[i]);
        }
    }
}
