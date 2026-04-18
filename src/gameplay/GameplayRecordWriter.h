#pragma once

#include "data/GameplayResult.h"

#include <cstdint>
#include <string>

class GameplayRecordWriter {
public:
    // Saves one final gameplay result for current authenticated user.
    // timestampSec=0 means using current unix timestamp.
    static bool saveFinalResult(const GameplayFinalResult &result,
                                const std::string &chartID,
                                const std::string &songName,
                                uint32_t timestampSec = 0
        );
};
