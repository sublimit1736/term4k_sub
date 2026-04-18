#pragma once

#include "data/GameplayChartData.h"

#include <cstdint>
#include <string>

class GameplayChartParser {
public:
    static bool parseChart(const std::string &chartFilePath,
                           uint16_t keyCount,
                           GameplayChartData &outChart
        );
};
