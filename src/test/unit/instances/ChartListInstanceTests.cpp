#include "catch_amalgamated.hpp"

#include "instances/ChartListInstance.h"
#include "TestSupport.h"

using namespace test_support;

namespace {
    std::string metadataJson(const std::string &id,
                             const std::string &name,
                             float difficulty,
                             const std::string &artist  = "artist",
                             const std::string &charter = "charter"
        ) {
        return std::string("{") +
               "\"id\":\"" + id + "\"," +
               "\"displayname\":\"" + name + "\"," +
               "\"artist\":\"" + artist + "\"," +
               "\"charter\":\"" + charter + "\"," +
               "\"BPM\":\"120\"," +
               "\"difficulty\":\"" + std::to_string(difficulty) + "\"" +
               "}";
    }
}

TEST_CASE (
"ChartListInstance refresh and sort orchestrate service behavior"
,
"[instances][ChartListInstance]"
)
 {
    TempDir temp("term4k_chart_list_instance");
    const auto chartsRoot = temp.path() / "charts";
    std::filesystem::create_directories(chartsRoot / "one");
    std::filesystem::create_directories(chartsRoot / "two");

    writeTextFile(chartsRoot / "one" / "meta.json", metadataJson("one", "BName", 9.0f));
    writeTextFile(chartsRoot / "one" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "one" / "music.ogg", "dummy");
    writeTextFile(chartsRoot / "two" / "meta.json", metadataJson("two", "AName", 3.0f));
    writeTextFile(chartsRoot / "two" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "two" / "music.ogg", "dummy");

    ChartListInstance instance;
    REQUIRE(instance.refresh(chartsRoot.string(), ""));
    REQUIRE(instance.items().size() == 2);
    REQUIRE(instance.failures().empty());
    REQUIRE(instance.filteredOrderedChartIDs() == instance.orderedChartIDs());

    instance.sort(ChartListSortKey::DisplayName, SortOrder::Ascending);
    REQUIRE(instance.items().at(instance.orderedChartIDs()[0]).chart.getDisplayName() == "AName");
    REQUIRE(instance.items().at(instance.orderedChartIDs()[1]).chart.getDisplayName() == "BName");

    instance.sort(ChartListSortKey::Difficulty, SortOrder::Descending);
    REQUIRE(instance.items().at(instance.orderedChartIDs()[0]).chart.getDifficulty() == Catch::Approx(9.0f));
    REQUIRE(instance.items().at(instance.orderedChartIDs()[1]).chart.getDifficulty() == Catch::Approx(3.0f));

    REQUIRE_FALSE(instance.refresh((temp.path() / "missing").string(), ""));
    REQUIRE(instance.items().empty());
    REQUIRE(instance.failures().empty());
}

TEST_CASE (
"ChartListInstance exposes detection failures for invalid chart folders"
,
"[instances][ChartListInstance]"
)
 {
    TempDir temp("term4k_chart_list_instance_failures");
    const auto chartsRoot = temp.path() / "charts";
    std::filesystem::create_directories(chartsRoot / "good");
    std::filesystem::create_directories(chartsRoot / "bad");

    writeTextFile(chartsRoot / "good" / "meta.json", metadataJson("good", "Good", 4.0f));
    writeTextFile(chartsRoot / "good" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "good" / "music.ogg", "dummy");

    writeTextFile(chartsRoot / "bad" / "meta.json", metadataJson("bad", "Bad", 8.0f));
    writeTextFile(chartsRoot / "bad" / "chart.t4k", "t4kcb\nt4kce\n");

    ChartListInstance instance;
    REQUIRE(instance.refresh(chartsRoot.string(), ""));
    REQUIRE(instance.items().size() == 1);
    REQUIRE(instance.failures().size() == 1);
    REQUIRE(instance.failures().front().issue == ChartDetectionIssue::MissingMusic);
}

TEST_CASE (
"ChartListInstance supports case-insensitive prefix search by selectable mode"
,
"[instances][ChartListInstance]"
)
 {
    TempDir temp("term4k_chart_list_instance_search");
    const auto chartsRoot = temp.path() / "charts";
    std::filesystem::create_directories(chartsRoot / "star");
    std::filesystem::create_directories(chartsRoot / "night");
    std::filesystem::create_directories(chartsRoot / "blue");
    std::filesystem::create_directories(chartsRoot / "signal");

    writeTextFile(chartsRoot / "star" / "meta.json", metadataJson("star", "Starfall", 10.0f, "Alpha", "Mika"));
    writeTextFile(chartsRoot / "star" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "star" / "music.ogg", "dummy");

    writeTextFile(chartsRoot / "night" / "meta.json", metadataJson("night", "Night", 5.0f, "Stellar", "Yuki"));
    writeTextFile(chartsRoot / "night" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "night" / "music.ogg", "dummy");

    writeTextFile(chartsRoot / "blue" / "meta.json", metadataJson("blue", "Blue", 7.0f, "Delta", "Stone"));
    writeTextFile(chartsRoot / "blue" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "blue" / "music.ogg", "dummy");

    writeTextFile(chartsRoot / "signal" / "meta.json", metadataJson("signal", "Signal", 12.0f, "Band", "Lime"));
    writeTextFile(chartsRoot / "signal" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "signal" / "music.ogg", "dummy");

    ChartListInstance instance;
    REQUIRE(instance.refresh(chartsRoot.string(), ""));

    REQUIRE(instance.searchMode() == ChartSearchMode::DisplayName);

    instance.setSearchQuery("STA");
    REQUIRE(instance.filteredOrderedChartIDs().size() == 1);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "star");

    instance.setSearchQuery("st");
    REQUIRE(instance.filteredOrderedChartIDs().size() == 1);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "star");

    instance.setSearchMode(ChartSearchMode::Artist);
    REQUIRE(instance.filteredOrderedChartIDs().size() == 1);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "night");

    instance.setSearchMode(ChartSearchMode::Charter);
    REQUIRE(instance.filteredOrderedChartIDs().size() == 1);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "blue");

    instance.setSearchMode(ChartSearchMode::Artist);
    instance.setSearchQuery("a");
    REQUIRE(instance.filteredOrderedChartIDs().size() == 1);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "star");

    instance.setSearchMode(ChartSearchMode::DisplayName);
    instance.setSearchQuery("s");
    REQUIRE(instance.filteredOrderedChartIDs().size() == 2);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "signal");
    REQUIRE(instance.filteredOrderedChartIDs()[1] == "star");

    instance.sort(ChartListSortKey::Difficulty, SortOrder::Descending);
    REQUIRE(instance.filteredOrderedChartIDs().size() == 2);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "signal");
    REQUIRE(instance.filteredOrderedChartIDs()[1] == "star");

    instance.setSearchMode(ChartSearchMode::Charter);
    instance.setSearchQuery("st");
    REQUIRE(instance.filteredOrderedChartIDs().size() == 1);
    REQUIRE(instance.filteredOrderedChartIDs()[0] == "blue");

    instance.clearSearch();
    REQUIRE(instance.searchQuery().empty());
    REQUIRE(instance.filteredOrderedChartIDs() == instance.orderedChartIDs());
}