#pragma once

#include "services/ChartCatalogService.h"

#include <string>
#include <vector>

class UserStatInstance {
public:
    bool refresh(const std::string &chartsRoot);

    const ChartRecordCollection &records() const;

    double rating() const;

    double potential() const;

private:
    static double singleChartEvaluation(float difficulty, float accuracy);

    ChartRecordCollection userRecords;
    double currentRating    = 0.0;
    double currentPotential = 0.0;
};