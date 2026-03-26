#pragma once

#include "services/ChartCatalogService.h"
#include "utils/PrefixTrie.h"

#include <string>
#include <vector>

enum class ChartSearchMode { DisplayName, Artist, Charter, };

class ChartListInstance {
public:
    bool refresh(const std::string &chartsRoot, const std::string &uid);

    void sort(ChartListSortKey key, SortOrder order);

    const ChartCatalogMap &items() const;

    const std::vector<std::string> &orderedChartIDs() const;

    // Display order under the current query condition.
    const std::vector<std::string> &filteredOrderedChartIDs() const;

    // Sets prefix query text (matched under current search mode only).
    void setSearchQuery(const std::string &query);

    void setSearchMode(ChartSearchMode mode);

    ChartSearchMode searchMode() const;

    // Clears search text and falls back to the full ordered list.
    void clearSearch();

    const std::string &searchQuery() const;

    const std::vector<ChartDetectionFailure> &failures() const;

private:
    void rebuildSearchIndex();

    void applySearchFilter();

    void resetSearchCursors();

    ChartCatalogMap chartItems;
    std::vector<std::string> orderedChartIds;
    std::vector<std::string> filteredOrderedChartIds;
    ChartSearchMode currentSearchMode = ChartSearchMode::DisplayName;
    std::string currentSearchQuery;
    PrefixTrie displayNameTrie;
    PrefixTrie artistTrie;
    PrefixTrie charterTrie;
    PrefixTrie::SearchCursor displayNameCursor;
    PrefixTrie::SearchCursor artistCursor;
    PrefixTrie::SearchCursor charterCursor;
    std::vector<ChartDetectionFailure> detectionFailures;
};
