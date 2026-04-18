#include "GameplayResult.h"

GameplaySnapshot::GameplaySnapshot() = default;

GameplaySnapshot::GameplaySnapshot(const uint32_t perfectCount, const uint32_t greatCount, const uint32_t missCount,
                                   const uint32_t earlyCount, const uint32_t lateCount, const uint64_t score,
                                   const double accuracy, const uint32_t currentCombo, const uint32_t maxCombo,
                                   const uint32_t chartNoteCount, const uint64_t chartMaxScore,
                                   const uint32_t currentChartTimeMs, const bool settlementAnimationTriggered,
                                   const bool resultReady
    ) : perfectCount_(perfectCount), greatCount_(greatCount), missCount_(missCount),
        earlyCount_(earlyCount), lateCount_(lateCount), score_(score), accuracy_(accuracy),
        currentCombo_(currentCombo), maxCombo_(maxCombo), chartNoteCount_(chartNoteCount),
        chartMaxScore_(chartMaxScore), currentChartTimeMs_(currentChartTimeMs),
        settlementAnimationTriggered_(settlementAnimationTriggered), resultReady_(resultReady) {}

uint32_t GameplaySnapshot::getPerfectCount() const { return perfectCount_; }
uint32_t GameplaySnapshot::getGreatCount() const { return greatCount_; }
uint32_t GameplaySnapshot::getMissCount() const { return missCount_; }
uint32_t GameplaySnapshot::getEarlyCount() const { return earlyCount_; }
uint32_t GameplaySnapshot::getLateCount() const { return lateCount_; }
uint64_t GameplaySnapshot::getScore() const { return score_; }
double GameplaySnapshot::getAccuracy() const { return accuracy_; }
uint32_t GameplaySnapshot::getCurrentCombo() const { return currentCombo_; }
uint32_t GameplaySnapshot::getMaxCombo() const { return maxCombo_; }
uint32_t GameplaySnapshot::getChartNoteCount() const { return chartNoteCount_; }
uint64_t GameplaySnapshot::getChartMaxScore() const { return chartMaxScore_; }
uint32_t GameplaySnapshot::getCurrentChartTimeMs() const { return currentChartTimeMs_; }
bool GameplaySnapshot::isSettlementAnimationTriggered() const { return settlementAnimationTriggered_; }
bool GameplaySnapshot::isResultReady() const { return resultReady_; }

void GameplaySnapshot::setPerfectCount(const uint32_t value) { perfectCount_ = value; }
void GameplaySnapshot::setGreatCount(const uint32_t value) { greatCount_ = value; }
void GameplaySnapshot::setMissCount(const uint32_t value) { missCount_ = value; }
void GameplaySnapshot::setEarlyCount(const uint32_t value) { earlyCount_ = value; }
void GameplaySnapshot::setLateCount(const uint32_t value) { lateCount_ = value; }
void GameplaySnapshot::setScore(const uint64_t value) { score_ = value; }
void GameplaySnapshot::setAccuracy(const double value) { accuracy_ = value; }
void GameplaySnapshot::setCurrentCombo(const uint32_t value) { currentCombo_ = value; }
void GameplaySnapshot::setMaxCombo(const uint32_t value) { maxCombo_ = value; }
void GameplaySnapshot::setChartNoteCount(const uint32_t value) { chartNoteCount_ = value; }
void GameplaySnapshot::setChartMaxScore(const uint64_t value) { chartMaxScore_ = value; }
void GameplaySnapshot::setCurrentChartTimeMs(const uint32_t value) { currentChartTimeMs_ = value; }
void GameplaySnapshot::setSettlementAnimationTriggered(const bool value) { settlementAnimationTriggered_ = value; }
void GameplaySnapshot::setResultReady(const bool value) { resultReady_ = value; }

GameplayFinalResult::GameplayFinalResult() = default;

GameplayFinalResult::GameplayFinalResult(const uint32_t perfectCount, const uint32_t greatCount,
                                         const uint32_t missCount, const uint32_t earlyCount,
                                         const uint32_t lateCount, const uint64_t score,
                                         const double accuracy, const uint32_t maxCombo,
                                         const uint32_t chartNoteCount, const uint64_t chartMaxScore
    ) : perfectCount_(perfectCount), greatCount_(greatCount), missCount_(missCount),
        earlyCount_(earlyCount), lateCount_(lateCount), score_(score), accuracy_(accuracy),
        maxCombo_(maxCombo), chartNoteCount_(chartNoteCount), chartMaxScore_(chartMaxScore) {}

uint32_t GameplayFinalResult::getPerfectCount() const { return perfectCount_; }
uint32_t GameplayFinalResult::getGreatCount() const { return greatCount_; }
uint32_t GameplayFinalResult::getMissCount() const { return missCount_; }
uint32_t GameplayFinalResult::getEarlyCount() const { return earlyCount_; }
uint32_t GameplayFinalResult::getLateCount() const { return lateCount_; }
uint64_t GameplayFinalResult::getScore() const { return score_; }
double GameplayFinalResult::getAccuracy() const { return accuracy_; }
uint32_t GameplayFinalResult::getMaxCombo() const { return maxCombo_; }
uint32_t GameplayFinalResult::getChartNoteCount() const { return chartNoteCount_; }
uint64_t GameplayFinalResult::getChartMaxScore() const { return chartMaxScore_; }

void GameplayFinalResult::setPerfectCount(const uint32_t value) { perfectCount_ = value; }
void GameplayFinalResult::setGreatCount(const uint32_t value) { greatCount_ = value; }
void GameplayFinalResult::setMissCount(const uint32_t value) { missCount_ = value; }
void GameplayFinalResult::setEarlyCount(const uint32_t value) { earlyCount_ = value; }
void GameplayFinalResult::setLateCount(const uint32_t value) { lateCount_ = value; }
void GameplayFinalResult::setScore(const uint64_t value) { score_ = value; }
void GameplayFinalResult::setAccuracy(const double value) { accuracy_ = value; }
void GameplayFinalResult::setMaxCombo(const uint32_t value) { maxCombo_ = value; }
void GameplayFinalResult::setChartNoteCount(const uint32_t value) { chartNoteCount_ = value; }
void GameplayFinalResult::setChartMaxScore(const uint64_t value) { chartMaxScore_ = value; }

GameplayClockSnapshot::GameplayClockSnapshot() = default;

GameplayClockSnapshot::GameplayClockSnapshot(const uint32_t audioTimeMs, const uint32_t chartTimeMs,
                                             const bool chartClockDrivenByAudio, const bool audioFinished
    ) : audioTimeMs_(audioTimeMs), chartTimeMs_(chartTimeMs),
        chartClockDrivenByAudio_(chartClockDrivenByAudio), audioFinished_(audioFinished) {}

uint32_t GameplayClockSnapshot::getAudioTimeMs() const { return audioTimeMs_; }
uint32_t GameplayClockSnapshot::getChartTimeMs() const { return chartTimeMs_; }
bool GameplayClockSnapshot::isChartClockDrivenByAudio() const { return chartClockDrivenByAudio_; }
bool GameplayClockSnapshot::isAudioFinished() const { return audioFinished_; }

void GameplayClockSnapshot::setAudioTimeMs(const uint32_t value) { audioTimeMs_ = value; }
void GameplayClockSnapshot::setChartTimeMs(const uint32_t value) { chartTimeMs_ = value; }
void GameplayClockSnapshot::setChartClockDrivenByAudio(const bool value) { chartClockDrivenByAudio_ = value; }
void GameplayClockSnapshot::setAudioFinished(const bool value) { audioFinished_ = value; }
