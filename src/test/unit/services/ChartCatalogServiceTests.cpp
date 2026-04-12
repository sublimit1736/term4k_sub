#include "catch_amalgamated.hpp"

#include "services/ChartCatalogService.h"
#include "dao/ProofedRecordsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

using namespace test_support;

namespace {
    std::string metadataJson(const std::string &id,
                             const std::string &name,
                             float difficulty
        ) {
        return std::string("{") +
               "\"id\":\"" + id + "\"," +
               "\"displayname\":\"" + name + "\"," +
               "\"artist\":\"artist\"," +
               "\"charter\":\"charter\"," +
               "\"BPM\":\"120\"," +
               "\"difficulty\":\"" + std::to_string(difficulty) + "\"," +
               "\"previewBegin\":\"0\"," +
               "\"previewEnd\":\"0\"," +
               "\"keyCount\":\"4\"," +
               "\"baseBPM\":\"120\"," +
               "\"baseTempo\":\"4,4\"" +
               "}";
    }
}

TEST_CASE (
"ChartCatalogService loads chart metadata and merges best stats"
,
"[services][ChartCatalogService]"
)
 {
    TempDir temp("term4k_chart_catalog");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot);
    std::filesystem::create_directories(dataRoot);

    const auto alphaDir = chartsRoot / "chart_alpha";
    const auto betaDir = chartsRoot / "chart_beta";
    std::filesystem::create_directories(alphaDir);
    std::filesystem::create_directories(betaDir);

    writeTextFile(alphaDir / "meta.json", metadataJson("chart_alpha", "Alpha", 8.5f));
    writeTextFile(betaDir / "meta.json", metadataJson("chart_beta", "Beta", 12.0f));
    writeTextFile(alphaDir / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(alphaDir / "music.ogg", "dummy");
    writeTextFile(betaDir / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(betaDir / "music.ogg", "dummy");

    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(ProofedRecordsDAO::addRecord("7 chart_alpha song userA 900000 97.50 1000 111"));
    REQUIRE(ProofedRecordsDAO::addRecord("7 chart_alpha song userA 880000 98.20 1200 222"));
    REQUIRE(ProofedRecordsDAO::addRecord("8 chart_alpha song userB 999999 99.99 1300 333"));

    const auto items = ChartCatalogService::loadCatalogForUID(chartsRoot.string(), "7");
    REQUIRE(items.size() == 2);

    const auto alphaIt = items.find("chart_alpha");
    REQUIRE(alphaIt != items.end());
    REQUIRE(alphaIt->second.stats.playCount == 2);
    REQUIRE(alphaIt->second.stats.bestScore == 900000);
    REQUIRE(alphaIt->second.stats.bestAccuracy == Catch::Approx(98.2f));
    REQUIRE_FALSE(alphaIt->second.chartFilePath.empty());
    REQUIRE_FALSE(alphaIt->second.musicFilePath.empty());

    const auto betaIt = items.find("chart_beta");
    REQUIRE(betaIt != items.end());
    REQUIRE(betaIt->second.stats.playCount == 0);
    REQUIRE(betaIt->second.stats.bestScore == 0);
    REQUIRE(betaIt->second.stats.bestAccuracy == Catch::Approx(0.0f));

    ProofedRecordsDAO::setDataDir(".");
}

TEST_CASE (
"ChartCatalogService ignores folders that violate fixed file/id rules"
,
"[services][ChartCatalogService]"
)
 {
    TempDir temp("term4k_chart_catalog_strict");
    const auto chartsRoot = temp.path() / "charts";
    std::filesystem::create_directories(chartsRoot);

    const auto validDir = chartsRoot / "good_chart";
    std::filesystem::create_directories(validDir);
    writeTextFile(validDir / "meta.json", metadataJson("good_chart", "Good", 5.0f));
    writeTextFile(validDir / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(validDir / "music.ogg", "dummy");

    const auto idMismatchDir = chartsRoot / "folder_name";
    std::filesystem::create_directories(idMismatchDir);
    writeTextFile(idMismatchDir / "meta.json", metadataJson("other_id", "Mismatch", 6.0f));
    writeTextFile(idMismatchDir / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(idMismatchDir / "music.ogg", "dummy");

    const auto missingFileDir = chartsRoot / "missing_music";
    std::filesystem::create_directories(missingFileDir);
    writeTextFile(missingFileDir / "meta.json", metadataJson("missing_music", "Missing", 7.0f));
    writeTextFile(missingFileDir / "chart.t4k", "t4kcb\nt4kce\n");

    std::vector<ChartDetectionFailure> failures;
    const auto items = ChartCatalogService::loadCatalogForUID(chartsRoot.string(), "", &failures);
    REQUIRE(items.size() == 1);
    REQUIRE(items.find("good_chart") != items.end());

    REQUIRE(failures.size() == 2);
    const auto mismatchIt = std::find_if(failures.begin(), failures.end(), [](const ChartDetectionFailure &f) {
        return f.issue == ChartDetectionIssue::FolderIDMismatch;
    });
    REQUIRE(mismatchIt != failures.end());
    REQUIRE(mismatchIt->metadataID == "other_id");
    REQUIRE(mismatchIt->localizedIssue == "chartdetect.issue.folder_id_mismatch");

    const auto missingMusicIt = std::find_if(failures.begin(), failures.end(), [](const ChartDetectionFailure &f) {
        return f.issue == ChartDetectionIssue::MissingMusic;
    });
    REQUIRE(missingMusicIt != failures.end());
    REQUIRE(missingMusicIt->localizedIssue == "chartdetect.issue.missing_music");
}

TEST_CASE (
"ChartCatalogService supports all requested sort keys and orders"
,
"[services][ChartCatalogService]"
)
 {
    ChartCatalogMap items;
    items["gamma"].chart.setDisplayName("Gamma");
    items["gamma"].chart.setDifficulty(7.0f);
    items["gamma"].stats.bestAccuracy = 90.0f;

    items["alpha"].chart.setDisplayName("alpha");
    items["alpha"].chart.setDifficulty(12.0f);
    items["alpha"].stats.bestAccuracy = 0.0f;

    items["beta"].chart.setDisplayName("Beta");
    items["beta"].chart.setDifficulty(3.0f);
    items["beta"].stats.bestAccuracy = 98.1f;

    auto sorted = ChartCatalogService::sortCatalogKeys(items, ChartListSortKey::DisplayName, SortOrder::Ascending);
    REQUIRE(items.at(sorted[0]).chart.getDisplayName() == "alpha");
    REQUIRE(items.at(sorted[1]).chart.getDisplayName() == "Beta");
    REQUIRE(items.at(sorted[2]).chart.getDisplayName() == "Gamma");

    sorted = ChartCatalogService::sortCatalogKeys(items, ChartListSortKey::Difficulty, SortOrder::Descending);
    REQUIRE(items.at(sorted[0]).chart.getDifficulty() == Catch::Approx(12.0f));
    REQUIRE(items.at(sorted[1]).chart.getDifficulty() == Catch::Approx(7.0f));
    REQUIRE(items.at(sorted[2]).chart.getDifficulty() == Catch::Approx(3.0f));

    sorted = ChartCatalogService::sortCatalogKeys(items, ChartListSortKey::BestAccuracy, SortOrder::Ascending);
    REQUIRE(items.at(sorted[0]).stats.bestAccuracy == Catch::Approx(0.0f));
    REQUIRE(items.at(sorted[1]).stats.bestAccuracy == Catch::Approx(90.0f));
    REQUIRE(items.at(sorted[2]).stats.bestAccuracy == Catch::Approx(98.1f));
}

TEST_CASE (
"ChartCatalogService compliance check reports playable-note conflicts"
,
"[services][ChartCatalogService]"
)
 {
    TempDir temp("term4k_chart_catalog_compliance");
    const auto chartPath = temp.path() / "chart.t4k";

    writeTextFile(chartPath,
                  std::string("t4kcb\n") +
                  "0100000003e8\n" + // tap lane0 t=1000
                  "0200000003e8000004b0\n" + // hold lane0 t=1000..1200 (tap vs hold head)
                  "02000000044c000003e8\n" + // hold lane0 t=1100..1000 (tap vs hold tail)
                  "t4kce\n");

    const auto conflicts = ChartCatalogService::checkChartCompliance(chartPath.string(), 1);
    REQUIRE(conflicts.size() == 2);

    REQUIRE(conflicts[0].lane == 0);
    REQUIRE(conflicts[0].timeMs == 1000);
    REQUIRE(conflicts[0].resolution == "keep_tap_drop_hold_head");

    REQUIRE(conflicts[1].lane == 0);
    REQUIRE(conflicts[1].timeMs == 1000);
    REQUIRE(conflicts[1].resolution == "keep_hold_tail_drop_tap");
}