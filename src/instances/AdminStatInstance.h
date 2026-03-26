#pragma once

#include "services/ChartCatalogService.h"

#include <map>
#include <string>
#include <vector>

enum class AdminRecordScope { VerifiedOnly, AllRecords };

struct AdminPlayerStats {
    std::string uid;
    ChartRecordCollection records;
    std::vector<double> b50;
    double rating    = 0.0;
    double potential = 0.0;

    std::map<std::string, ChartRecordCollection> recordsByChart;
};

class AdminStatInstance {
public:
    bool refresh(const std::string &chartsRoot);

    const std::map<std::string, AdminPlayerStats> &playerStats(AdminRecordScope scope) const;

    const ChartRecordCollection *recordsByUser(AdminRecordScope scope, const std::string &uid) const;

    const ChartRecordCollection *recordsByUserAndChart(AdminRecordScope scope,
                                                       const std::string &uid,
                                                       const std::string &chartId
        ) const;

private:
    std::map<std::string, AdminPlayerStats> verifiedStats;
    std::map<std::string, AdminPlayerStats> allStats;
};