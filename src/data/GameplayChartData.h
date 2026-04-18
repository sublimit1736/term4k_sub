#pragma once

#include <cstdint>
#include <vector>

class GameplayTapNote {
public:
    GameplayTapNote();

    GameplayTapNote(uint8_t lane, uint32_t timeMs);

    uint8_t getLane() const;

    uint32_t getTimeMs() const;

    void setLane(uint8_t value);

    void setTimeMs(uint32_t value);

private:
    uint8_t lane_    = 0;
    uint32_t timeMs_ = 0;
};

class GameplayHoldNote {
public:
    GameplayHoldNote();

    GameplayHoldNote(uint8_t lane, uint32_t headTimeMs, uint32_t tailTimeMs);

    uint8_t getLane() const;

    uint32_t getHeadTimeMs() const;

    uint32_t getTailTimeMs() const;

    void setLane(uint8_t value);

    void setHeadTimeMs(uint32_t value);

    void setTailTimeMs(uint32_t value);

private:
    uint8_t lane_        = 0;
    uint32_t headTimeMs_ = 0;
    uint32_t tailTimeMs_ = 0;
};

class GameplayChartData {
public:
    GameplayChartData();

    GameplayChartData(std::vector<GameplayTapNote> taps, std::vector<GameplayHoldNote> holds,
                      uint32_t endTimeMs, uint32_t noteCount, uint64_t maxScore
        );

    const std::vector<GameplayTapNote> &getTaps() const;

    const std::vector<GameplayHoldNote> &getHolds() const;

    uint32_t getEndTimeMs() const;

    uint32_t getNoteCount() const;

    uint64_t getMaxScore() const;

    std::vector<GameplayTapNote> &mutableTaps();

    std::vector<GameplayHoldNote> &mutableHolds();

    void setTaps(const std::vector<GameplayTapNote> &value);

    void setHolds(const std::vector<GameplayHoldNote> &value);

    void setEndTimeMs(uint32_t value);

    void setNoteCount(uint32_t value);

    void setMaxScore(uint64_t value);

private:
    std::vector<GameplayTapNote> taps_;
    std::vector<GameplayHoldNote> holds_;
    uint32_t endTimeMs_ = 0;
    uint32_t noteCount_ = 0;
    uint64_t maxScore_  = 0;
};
