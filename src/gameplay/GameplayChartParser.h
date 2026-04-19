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

    // Returns the minimum key count (max lane + 1) inferred from the chart file,
    // or 0 if the file cannot be read or contains no playable notes.
    static uint16_t detectKeyCount(const std::string &chartFilePath);
};
