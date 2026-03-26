#include "AdminStatInstance.h"

#include "services/AuthenticatedUserService.h"
#include "services/ChartCatalogService.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace {
    struct ParsedRecord {
        std::string uid;
        std::string chartId;
        uint32_t score     = 0;
        float accuracy     = 0.0f;
        uint32_t timestamp = 0;
        bool valid         = false;
    };

    bool isDigitsOnly(const std::string &value) {
        return !value.empty() && value.find_first_not_of("0123456789") == std::string::npos;
    }

    ParsedRecord parseRecord(const std::string &record) {
        std::istringstream iss(record);
        std::vector<std::string> fields;
        std::string token;
        while (iss >> token){
            fields.push_back(token);
        }

        ParsedRecord parsed;
        if (fields.size() < 6) return parsed;

        const bool uidFormat = (fields.size() >= 7) && isDigitsOnly(fields[0]);
        if (!uidFormat) return parsed;

        try{
            parsed.uid       = fields.at(0);
            parsed.chartId   = fields.at(1);
            parsed.score     = static_cast<uint32_t>(std::stoul(fields.at(4)));
            parsed.accuracy  = std::stof(fields.at(5));
            parsed.timestamp = static_cast<uint32_t>(std::stoul(fields.at(6)));
            parsed.valid     = true;
        }
        catch (...){
            parsed.valid = false;
        }
        return parsed;
    }

    double normalizeAccuracy(const float accuracy) {
        if (accuracy > 1.0f) return static_cast<double>(accuracy) / 100.0;
        if (accuracy < 0.0f) return 0.0;
        return static_cast<double>(accuracy);
    }

    double evaluateSingle(const float difficulty, const float accuracy) {
        const double a  = normalizeAccuracy(accuracy);
        const double a2 = a * a;
        const double a4 = a2 * a2;
        return static_cast<double>(difficulty) * (a - a2 + a4);
    }

    ChartRecordEntry buildRecordEntry(const ParsedRecord &parsed,
                                      const std::map<std::string, ChartCatalogEntry> &catalogById
        ) {
        ChartRecordEntry item;
        item.uid       = parsed.uid;
        item.chartID   = parsed.chartId;
        item.score     = parsed.score;
        item.accuracy  = parsed.accuracy;
        item.timestamp = parsed.timestamp;

        const auto found = catalogById.find(parsed.chartId);
        if (found != catalogById.end()){
            item.chart = found->second.chart;
        }
        else{
            item.chart.setID(parsed.chartId);
            item.chart.setDisplayName(parsed.chartId);
        }
        return item;
    }

    std::map<std::string, AdminPlayerStats> buildStats(const std::vector<std::string> &records,
                                                       const std::string &chartsRoot
        ) {
        // Group by UID first, then sort and compute B50 per user.
        std::map<std::string, std::vector<ParsedRecord>> parsedByUid;
        for (const auto &record: records){
            const ParsedRecord parsed = parseRecord(record);
            if (!parsed.valid) continue;
            parsedByUid[parsed.uid].push_back(parsed);
        }

        std::map<std::string, AdminPlayerStats> out;
        for (auto &[uid, parsedRecords]: parsedByUid){
            const auto catalogById = ChartCatalogService::loadCatalogForUID(chartsRoot, uid);

            std::stable_sort(parsedRecords.begin(), parsedRecords.end(),
                             [](const ParsedRecord &a, const ParsedRecord &b) {
                                 return a.timestamp > b.timestamp;
                             });

            AdminPlayerStats stats;
            stats.uid = uid;

            std::vector<double> evaluations;
            evaluations.reserve(parsedRecords.size());

            std::size_t serial = 0;
            for (const auto &parsed: parsedRecords){
                ChartRecordEntry item = buildRecordEntry(parsed, catalogById);
                evaluations.push_back(evaluateSingle(item.chart.getDifficulty(), item.accuracy));

                const std::string key      = std::to_string(parsed.timestamp) + "#" + std::to_string(serial++);
                stats.records.records[key] = item;
                stats.records.order.push_back(key);

                auto &byChart        = stats.recordsByChart[parsed.chartId];
                byChart.records[key] = std::move(item);
                byChart.order.push_back(key);
            }

            std::sort(evaluations.begin(), evaluations.end(), std::greater<double>());
            // B50 takes top 50 single-chart evaluations; potential is their average.
            const std::size_t b50Count = std::min<std::size_t>(50, evaluations.size());
            stats.b50.reserve(b50Count);
            for (std::size_t i = 0; i < b50Count; ++i){
                stats.b50.push_back(evaluations[i]);
                stats.rating += evaluations[i];
            }
            if (b50Count > 0){
                stats.potential = stats.rating / static_cast<double>(b50Count);
            }

            out.emplace(uid, std::move(stats));
        }

        return out;
    }
}

bool AdminStatInstance::refresh(const std::string &chartsRoot) {
    if (!AuthenticatedUserService::isAdminUser()){
        verifiedStats.clear();
        allStats.clear();
        return false;
    }

    verifiedStats = buildStats(AuthenticatedUserService::loadAllVerifiedRecords(), chartsRoot);
    allStats      = buildStats(AuthenticatedUserService::loadAllRecords(), chartsRoot);
    return true;
}

const std::map<std::string, AdminPlayerStats> &AdminStatInstance::playerStats(const AdminRecordScope scope) const {
    return scope == AdminRecordScope::VerifiedOnly ? verifiedStats : allStats;
}

const ChartRecordCollection *AdminStatInstance::recordsByUser(const AdminRecordScope scope,
                                                              const std::string &uid
    ) const {
    const auto &source = playerStats(scope);
    const auto it      = source.find(uid);
    if (it == source.end()) return nullptr;
    return &it->second.records;
}

const ChartRecordCollection *AdminStatInstance::recordsByUserAndChart(const AdminRecordScope scope,
                                                                      const std::string &uid,
                                                                      const std::string &chartId
    ) const {
    const auto &source = playerStats(scope);
    const auto userIt  = source.find(uid);
    if (userIt == source.end()) return nullptr;

    const auto chartIt = userIt->second.recordsByChart.find(chartId);
    if (chartIt == userIt->second.recordsByChart.end()) return nullptr;
    return &chartIt->second;
}