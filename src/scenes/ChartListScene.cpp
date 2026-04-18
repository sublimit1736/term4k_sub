#include "ChartListScene.h"

#include "chart/ChartCatalog.h"

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

bool ChartListScene::refresh(const std::string &chartsRoot, const std::string &uid) {
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

    chartItems      = ChartCatalog::loadCatalogForUID(chartsRoot, uid, &detectionFailures);
    orderedChartIds =
        ChartCatalog::sortCatalogKeys(chartItems, ChartListSortKey::DisplayName, SortOrder::Ascending);
    rebuildSearchIndex();
    applySearchFilter();

    return true;
}

void ChartListScene::sort(ChartListSortKey key, SortOrder order) {
    orderedChartIds = ChartCatalog::sortCatalogKeys(chartItems, key, order);
    rebuildSearchIndex();
    applySearchFilter();
}

const ChartCatalogMap &ChartListScene::items() const {
    return chartItems;
}

const std::vector<std::string> &ChartListScene::orderedChartIDs() const {
    return orderedChartIds;
}

const std::vector<std::string> &ChartListScene::filteredOrderedChartIDs() const {
    return filteredOrderedChartIds;
}

void ChartListScene::setSearchQuery(const std::string &query) {
    currentSearchQuery = trim(query);
    applySearchFilter();
}

void ChartListScene::setSearchMode(ChartSearchMode mode) {
    if (currentSearchMode == mode) return;
    currentSearchMode = mode;
    applySearchFilter();
}

ChartSearchMode ChartListScene::searchMode() const {
    return currentSearchMode;
}

void ChartListScene::clearSearch() {
    currentSearchQuery.clear();
    applySearchFilter();
}

const std::string &ChartListScene::searchQuery() const {
    return currentSearchQuery;
}

const std::vector<ChartDetectionFailure> &ChartListScene::failures() const {
    return detectionFailures;
}

void ChartListScene::rebuildSearchIndex() {
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

void ChartListScene::resetSearchCursors() {
    displayNameCursor = PrefixTrie::SearchCursor{};
    artistCursor      = PrefixTrie::SearchCursor{};
    charterCursor     = PrefixTrie::SearchCursor{};
}

void ChartListScene::applySearchFilter() {
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
