#include "GameplayStatsService.h"

#include <algorithm>

void GameplayStatsService::reset(const uint32_t noteCount, const uint64_t maxScore) {
    perfectCount_ = 0;
    greatCount_   = 0;
    missCount_    = 0;
    earlyCount_   = 0;
    lateCount_    = 0;

    score_          = 0;
    accuracyPoints_ = 0.0;
    accuracy_       = 0.0;

    currentCombo_ = 0;
    maxCombo_     = 0;

    chartNoteCount_ = noteCount;
    chartMaxScore_  = maxScore;
    updateAccuracy();
}

void GameplayStatsService::settleNote(const GameplayJudgement judgement, const bool isEarly, const bool isLate) {
    if (isEarly) ++earlyCount_;
    if (isLate) ++lateCount_;

    const uint32_t comboBefore = currentCombo_;

    switch (judgement){
        case GameplayJudgement::Perfect: ++perfectCount_;
            score_          += 2ULL + static_cast<uint64_t>(comboBefore);
            accuracyPoints_ += 1.0;
            ++currentCombo_;
            maxCombo_ = std::max(maxCombo_, currentCombo_);
            break;
        case GameplayJudgement::Great: ++greatCount_;
            score_          += 1ULL + static_cast<uint64_t>(comboBefore / 2U);
            accuracyPoints_ += 0.5;
            ++currentCombo_;
            maxCombo_ = std::max(maxCombo_, currentCombo_);
            break;
        case GameplayJudgement::Miss: ++missCount_;
            currentCombo_ = 0;
            break;
    }

    updateAccuracy();
}

uint32_t GameplayStatsService::chartNoteCount() const {
    return chartNoteCount_;
}

uint64_t GameplayStatsService::chartMaxScore() const {
    return chartMaxScore_;
}

GameplaySnapshot GameplayStatsService::buildSnapshot(const uint32_t chartTimeMs,
                                                     const bool settlementAnimationTriggered,
                                                     const bool resultReady
    ) const {
    GameplaySnapshot s;
    s.setPerfectCount(perfectCount_);
    s.setGreatCount(greatCount_);
    s.setMissCount(missCount_);
    s.setEarlyCount(earlyCount_);
    s.setLateCount(lateCount_);

    s.setScore(score_);
    s.setAccuracy(accuracy_);

    s.setCurrentCombo(currentCombo_);
    s.setMaxCombo(maxCombo_);

    s.setChartNoteCount(chartNoteCount_);
    s.setChartMaxScore(chartMaxScore_);

    s.setCurrentChartTimeMs(chartTimeMs);
    s.setSettlementAnimationTriggered(settlementAnimationTriggered);
    s.setResultReady(resultReady);
    return s;
}

GameplayFinalResult GameplayStatsService::buildFinalResult() const {
    GameplayFinalResult r;
    r.setPerfectCount(perfectCount_);
    r.setGreatCount(greatCount_);
    r.setMissCount(missCount_);
    r.setEarlyCount(earlyCount_);
    r.setLateCount(lateCount_);

    r.setScore(score_);
    r.setAccuracy(accuracy_);
    r.setMaxCombo(maxCombo_);

    r.setChartNoteCount(chartNoteCount_);
    r.setChartMaxScore(chartMaxScore_);
    return r;
}

void GameplayStatsService::updateAccuracy() {
    if (chartNoteCount_ == 0){
        accuracy_ = 0.0;
        return;
    }
    accuracy_ = std::clamp((accuracyPoints_ * 100.0) / static_cast<double>(chartNoteCount_), 0.0, 100.0);
}
