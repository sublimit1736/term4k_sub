#pragma once

#include "entities/GameplayResult.h"

#include <cstdint>
#include <string>

class GameplaySessionService;

class GameplaySettlementInstance {
public:
    // Enter settlement state and auto-trigger one record save.
    // Re-enter is rejected to prevent manual repeated save.
    bool onEnterSettlement(const GameplaySessionService &session,
                           const std::string &chartID,
                           const std::string &songName,
                           uint32_t timestampSec = 0
        );

    bool hasFinalResult() const;

    bool recordSaveSucceeded() const;

    GameplayFinalResult finalResult() const;

private:
    GameplayFinalResult result_{};
    bool hasResult_     = false;
    bool saveTriggered_ = false;
    bool saveSucceeded_ = false;
};
