#pragma once

#include "data/GameplayResult.h"

#include <cstdint>

class GameplayStats {
public:
    void reset(uint32_t noteCount, uint64_t maxScore);

    void settleNote(GameplayJudgement judgement, bool isEarly, bool isLate);

    uint32_t chartNoteCount() const;

    uint64_t chartMaxScore() const;

    GameplaySnapshot buildSnapshot(uint32_t chartTimeMs,
                                   bool settlementAnimationTriggered,
                                   bool resultReady
        ) const;

    GameplayFinalResult buildFinalResult() const;

private:
    void updateAccuracy();

    uint32_t perfectCount_ = 0;
    uint32_t greatCount_   = 0;
    uint32_t missCount_    = 0;
    uint32_t earlyCount_   = 0;
    uint32_t lateCount_    = 0;

    uint64_t score_        = 0;
    double accuracyPoints_ = 0.0;
    double accuracy_       = 0.0;

    uint32_t currentCombo_ = 0;
    uint32_t maxCombo_     = 0;

    uint32_t chartNoteCount_ = 0;
    uint64_t chartMaxScore_  = 0;
};
