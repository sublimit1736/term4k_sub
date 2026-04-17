#pragma once

#include "entities/GameplayChartData.h"

#include <cstdint>
#include <string>

class GameplayChartService {
public:
    static bool parseChart(const std::string &chartFilePath,
                           uint16_t keyCount,
                           GameplayChartData &outChart
        );
};
