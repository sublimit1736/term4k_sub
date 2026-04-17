#pragma once

#include "entities/GameplayResult.h"
#include "services/GameplayClockService.h"
#include "services/GameplayStatsService.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class GameplaySessionService {
public:
    bool openChart(const std::string &chartFilePath, uint16_t keyCount);

    void reset();

    void advanceChartTimeMs(uint32_t chartTimeMs);

    void advanceAudioTimeMs(uint32_t audioTimeMs);

    void setChartClockDrivenByAudio(bool enabled);

    void onKeyDown(uint8_t keyCode);

    void onKeyUp(uint8_t keyCode);

    void setAudioFinished(bool finished);

    GameplayClockSnapshot clockSnapshot() const;

    GameplaySnapshot snapshot() const;

    bool isResultReady() const;

    GameplayFinalResult finalResult() const;

    uint32_t chartNoteCount() const;

    uint64_t chartMaxScore() const;

private:
    struct TapNote {
        uint8_t lane    = 0;
        uint32_t timeMs = 0;
        bool resolved   = false;
    };

    struct HoldNote {
        uint8_t lane        = 0;
        uint32_t headTimeMs = 0;
        uint32_t tailTimeMs = 0;
        bool resolved       = false;
        bool headPerfect    = false;
    };

    struct LaneState {
        std::vector<std::size_t> tapOrder;
        std::vector<std::size_t> holdOrder;

        std::size_t tapLoadPos  = 0;
        std::size_t holdLoadPos = 0;

        std::size_t pendingTapPos  = 0;
        std::size_t pendingHoldPos = 0;

        std::vector<std::size_t> activeHoldIndices;
        bool isPressed = false;
    };

    std::optional<uint8_t> laneByKey(uint8_t keyCode) const;

    void loadWindowForLane(uint8_t lane);

    void loadWindowAllLanes();

    void settleTapByDelta(std::size_t tapIndex, int32_t deltaMs);

    void settleHoldHeadMiss(std::size_t holdIndex);

    void settleHoldTail(std::size_t holdIndex, GameplayJudgement judgement);

    void cleanupResolvedFront(uint8_t lane);

    void autoJudgeByTime(uint8_t lane);

    void settleIfReady();

    void rebuildLaneOrders();

    std::vector<TapNote> taps_;
    std::vector<HoldNote> holds_;
    std::vector<LaneState> lanes_;
    std::unordered_map<uint8_t, uint8_t> keyToLane_;

    GameplayClockService clock_;
    GameplayStatsService stats_;

    bool settlementAnimationTriggered_ = false;
    bool resultReady_                  = false;
    uint32_t chartEndTimeMs_           = 0;
};
