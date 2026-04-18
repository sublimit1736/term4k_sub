#include "catch_amalgamated.hpp"

#include "models/AdminStatInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/UserLoginService.h"
#include "dao/ProofedRecordsDAO.h"
#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

#include <fstream>

using namespace test_support;

namespace {
    std::string metadataJson(const std::string &id,
                             const std::string &name,
                             const float difficulty
        ) {
        return std::string("{") +
               "\"id\":\"" + id + "\"," +
               "\"displayname\":\"" + name + "\"," +
               "\"artist\":\"artist\"," +
               "\"charter\":\"charter\"," +
               "\"BPM\":\"120\"," +
               "\"difficulty\":\"" + std::to_string(difficulty) + "\"" +
               "}";
    }

    void appendUnverifiedRecord(const std::filesystem::path &recordsPath,
                                const std::string &plainRecord
        ) {
        std::vector<uint8_t> bytes(plainRecord.begin(), plainRecord.end());
        LiteDBUtils::xorObfuscate(bytes);
        const auto encrypted = LiteDBUtils::aesEncrypt(bytes);

        std::ofstream out(recordsPath, std::ios::app);
        out << LiteDBUtils::hexEncode(encrypted) << "\n";
    }
}

TEST_CASE (

"AdminStatInstance exposes per-user stats for verified and all records"
,
"[instances][AdminStatInstance]"
)
 {
    TempDir temp("term4k_admin_stat_instance");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot / "chart_a");
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartsRoot / "chart_a" / "meta.json", metadataJson("chart_a", "A", 8.0f));
    writeTextFile(chartsRoot / "chart_a" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "chart_a" / "music.ogg", "dummy");

    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 900000 90.0 100 300"));
    REQUIRE(ProofedRecordsDAO::addRecord("1001 chart_a song bob 880000 80.0 200 250"));

    appendUnverifiedRecord(dataRoot / "records.db", "1000 chart_a song alice 990000 99.0 999 500");

    UserLoginService::loginAdmin();
    REQUIRE(AuthenticatedUserService::syncFromUserLoginService());

    AdminStatInstance stat;
    REQUIRE(stat.refresh(chartsRoot.string()));

    const auto &verified = stat.playerStats(AdminRecordScope::VerifiedOnly);
    const auto &all = stat.playerStats(AdminRecordScope::AllRecords);

    REQUIRE(verified.find("1000") != verified.end());
    REQUIRE(all.find("1000") != all.end());
    REQUIRE(verified.at("1000").records.records.size() == 1);
    REQUIRE(all.at("1000").records.records.size() == 2);

    const auto userAllRecords = stat.recordsByUser(AdminRecordScope::AllRecords, "1000");
    REQUIRE(userAllRecords != nullptr);
    REQUIRE(userAllRecords->records.size() == 2);

    const auto byChart = stat.recordsByUserAndChart(AdminRecordScope::AllRecords, "1000", "chart_a");
    REQUIRE(byChart != nullptr);
    REQUIRE(byChart->records.size() == 2);
    REQUIRE(byChart->records.at(byChart->order[0]).maxCombo == 500);
    REQUIRE(byChart->records.at(byChart->order[1]).maxCombo == 300);

    AuthenticatedUserService::logout();
    UserLoginService::logout();
    ProofedRecordsDAO::setDataDir(".");
}

TEST_CASE (

"AdminStatInstance keeps stable order for equal timestamps and provides fallback chart metadata"
,
"[instances][AdminStatInstance]"
)
 {
    TempDir temp("term4k_admin_stat_instance_stable_fallback");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot / "chart_a");
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartsRoot / "chart_a" / "meta.json", metadataJson("chart_a", "A", 8.0f));
    writeTextFile(chartsRoot / "chart_a" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "chart_a" / "music.ogg", "dummy");

    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 123456 90.0 500 10"));
    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_missing song alice 654321 91.0 500 20"));

    UserLoginService::loginAdmin();
    REQUIRE(AuthenticatedUserService::syncFromUserLoginService());

    AdminStatInstance stat;
    REQUIRE(stat.refresh(chartsRoot.string()));

    const auto allRecords = stat.recordsByUser(AdminRecordScope::AllRecords, "1000");
    REQUIRE(allRecords != nullptr);
    REQUIRE(allRecords->order.size() == 2);

    const auto &first = allRecords->records.at(allRecords->order[0]);
    const auto &second = allRecords->records.at(allRecords->order[1]);
    REQUIRE(first.score == 123456);
    REQUIRE(second.score == 654321);
    REQUIRE(first.maxCombo == 10);
    REQUIRE(second.maxCombo == 20);
    REQUIRE(second.chart.getDisplayName() == "chart_missing");

    const auto byMissingChart = stat.recordsByUserAndChart(AdminRecordScope::AllRecords, "1000", "chart_missing");
    REQUIRE(byMissingChart != nullptr);
    REQUIRE(byMissingChart->records.size() == 1);
    REQUIRE(byMissingChart->records.begin()->second.chart.getDisplayName() == "chart_missing");

    AuthenticatedUserService::logout();
    UserLoginService::logout();
    ProofedRecordsDAO::setDataDir(".");
}

TEST_CASE (

"AdminStatInstance keeps maxCombo at 0 for legacy record format"
,
"[instances][AdminStatInstance]"
)
 {
    TempDir temp("term4k_admin_stat_instance_legacy_record");
    const auto chartsRoot = temp.path() / "charts";
    const auto dataRoot = temp.path() / "data";
    std::filesystem::create_directories(chartsRoot / "chart_a");
    std::filesystem::create_directories(dataRoot);

    writeTextFile(chartsRoot / "chart_a" / "meta.json", metadataJson("chart_a", "A", 8.0f));
    writeTextFile(chartsRoot / "chart_a" / "chart.t4k", "t4kcb\nt4kce\n");
    writeTextFile(chartsRoot / "chart_a" / "music.ogg", "dummy");

    ProofedRecordsDAO::setDataDir(dataRoot.string());
    LiteDBUtils::setKeyFile((dataRoot / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    // Legacy 7-field record without maxCombo.
    REQUIRE(ProofedRecordsDAO::addRecord("1000 chart_a song alice 900000 90.0 100"));

    UserLoginService::loginAdmin();
    REQUIRE(AuthenticatedUserService::syncFromUserLoginService());

    AdminStatInstance stat;
    REQUIRE(stat.refresh(chartsRoot.string()));

    const auto allRecords = stat.recordsByUser(AdminRecordScope::AllRecords, "1000");
    REQUIRE(allRecords != nullptr);
    REQUIRE(allRecords->order.size() == 1);
    REQUIRE(allRecords->records.at(allRecords->order[0]).maxCombo == 0);

    AuthenticatedUserService::logout();
    UserLoginService::logout();
    ProofedRecordsDAO::setDataDir(".");
}