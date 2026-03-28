#include "GameplayChartData.h"

GameplayTapNote::GameplayTapNote() = default;

GameplayTapNote::GameplayTapNote(const uint8_t lane, const uint32_t timeMs) : lane_(lane), timeMs_(timeMs) {}

uint8_t GameplayTapNote::getLane() const { return lane_; }
uint32_t GameplayTapNote::getTimeMs() const { return timeMs_; }

void GameplayTapNote::setLane(const uint8_t value) { lane_ = value; }
void GameplayTapNote::setTimeMs(const uint32_t value) { timeMs_ = value; }

GameplayHoldNote::GameplayHoldNote() = default;

GameplayHoldNote::GameplayHoldNote(const uint8_t lane, const uint32_t headTimeMs, const uint32_t tailTimeMs) :
    lane_(lane), headTimeMs_(headTimeMs), tailTimeMs_(tailTimeMs) {}

uint8_t GameplayHoldNote::getLane() const { return lane_; }
uint32_t GameplayHoldNote::getHeadTimeMs() const { return headTimeMs_; }
uint32_t GameplayHoldNote::getTailTimeMs() const { return tailTimeMs_; }

void GameplayHoldNote::setLane(const uint8_t value) { lane_ = value; }
void GameplayHoldNote::setHeadTimeMs(const uint32_t value) { headTimeMs_ = value; }
void GameplayHoldNote::setTailTimeMs(const uint32_t value) { tailTimeMs_ = value; }

GameplayChartData::GameplayChartData() = default;

GameplayChartData::GameplayChartData(std::vector<GameplayTapNote> taps,
                                     std::vector<GameplayHoldNote> holds,
                                     const uint32_t endTimeMs,
                                     const uint32_t noteCount,
                                     const uint64_t maxScore
    ) : taps_(std::move(taps)), holds_(std::move(holds)), endTimeMs_(endTimeMs),
        noteCount_(noteCount), maxScore_(maxScore) {}

const std::vector<GameplayTapNote> &GameplayChartData::getTaps() const { return taps_; }
const std::vector<GameplayHoldNote> &GameplayChartData::getHolds() const { return holds_; }
uint32_t GameplayChartData::getEndTimeMs() const { return endTimeMs_; }
uint32_t GameplayChartData::getNoteCount() const { return noteCount_; }
uint64_t GameplayChartData::getMaxScore() const { return maxScore_; }

std::vector<GameplayTapNote> &GameplayChartData::mutableTaps() { return taps_; }
std::vector<GameplayHoldNote> &GameplayChartData::mutableHolds() { return holds_; }

void GameplayChartData::setTaps(const std::vector<GameplayTapNote> &value) { taps_ = value; }
void GameplayChartData::setHolds(const std::vector<GameplayHoldNote> &value) { holds_ = value; }
void GameplayChartData::setEndTimeMs(const uint32_t value) { endTimeMs_ = value; }
void GameplayChartData::setNoteCount(const uint32_t value) { noteCount_ = value; }
void GameplayChartData::setMaxScore(const uint64_t value) { maxScore_ = value; }

