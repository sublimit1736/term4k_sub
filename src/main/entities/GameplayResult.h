#pragma once

#include <cstdint>

enum class GameplayJudgement { Perfect, Great, Miss, };

class GameplaySnapshot {
public:
    GameplaySnapshot();

    GameplaySnapshot(uint32_t perfectCount, uint32_t greatCount, uint32_t missCount,
                     uint32_t earlyCount, uint32_t lateCount, uint64_t score, double accuracy,
                     uint32_t currentCombo, uint32_t maxCombo,
                     uint32_t chartNoteCount, uint64_t chartMaxScore,
                     uint32_t currentChartTimeMs, bool settlementAnimationTriggered,
                     bool resultReady
        );

    uint32_t getPerfectCount() const;

    uint32_t getGreatCount() const;

    uint32_t getMissCount() const;

    uint32_t getEarlyCount() const;

    uint32_t getLateCount() const;

    uint64_t getScore() const;

    double getAccuracy() const;

    uint32_t getCurrentCombo() const;

    uint32_t getMaxCombo() const;

    uint32_t getChartNoteCount() const;

    uint64_t getChartMaxScore() const;

    uint32_t getCurrentChartTimeMs() const;

    bool isSettlementAnimationTriggered() const;

    bool isResultReady() const;

    void setPerfectCount(uint32_t value);

    void setGreatCount(uint32_t value);

    void setMissCount(uint32_t value);

    void setEarlyCount(uint32_t value);

    void setLateCount(uint32_t value);

    void setScore(uint64_t value);

    void setAccuracy(double value);

    void setCurrentCombo(uint32_t value);

    void setMaxCombo(uint32_t value);

    void setChartNoteCount(uint32_t value);

    void setChartMaxScore(uint64_t value);

    void setCurrentChartTimeMs(uint32_t value);

    void setSettlementAnimationTriggered(bool value);

    void setResultReady(bool value);

private:
    uint32_t perfectCount_             = 0;
    uint32_t greatCount_               = 0;
    uint32_t missCount_                = 0;
    uint32_t earlyCount_               = 0;
    uint32_t lateCount_                = 0;
    uint64_t score_                    = 0;
    double accuracy_                   = 0.0;
    uint32_t currentCombo_             = 0;
    uint32_t maxCombo_                 = 0;
    uint32_t chartNoteCount_           = 0;
    uint64_t chartMaxScore_            = 0;
    uint32_t currentChartTimeMs_       = 0;
    bool settlementAnimationTriggered_ = false;
    bool resultReady_                  = false;
};

class GameplayFinalResult {
public:
    GameplayFinalResult();

    GameplayFinalResult(uint32_t perfectCount, uint32_t greatCount, uint32_t missCount,
                        uint32_t earlyCount, uint32_t lateCount, uint64_t score,
                        double accuracy, uint32_t maxCombo,
                        uint32_t chartNoteCount, uint64_t chartMaxScore
        );

    uint32_t getPerfectCount() const;

    uint32_t getGreatCount() const;

    uint32_t getMissCount() const;

    uint32_t getEarlyCount() const;

    uint32_t getLateCount() const;

    uint64_t getScore() const;

    double getAccuracy() const;

    uint32_t getMaxCombo() const;

    uint32_t getChartNoteCount() const;

    uint64_t getChartMaxScore() const;

    void setPerfectCount(uint32_t value);

    void setGreatCount(uint32_t value);

    void setMissCount(uint32_t value);

    void setEarlyCount(uint32_t value);

    void setLateCount(uint32_t value);

    void setScore(uint64_t value);

    void setAccuracy(double value);

    void setMaxCombo(uint32_t value);

    void setChartNoteCount(uint32_t value);

    void setChartMaxScore(uint64_t value);

private:
    uint32_t perfectCount_   = 0;
    uint32_t greatCount_     = 0;
    uint32_t missCount_      = 0;
    uint32_t earlyCount_     = 0;
    uint32_t lateCount_      = 0;
    uint64_t score_          = 0;
    double accuracy_         = 0.0;
    uint32_t maxCombo_       = 0;
    uint32_t chartNoteCount_ = 0;
    uint64_t chartMaxScore_  = 0;
};

class GameplayClockSnapshot {
public:
    GameplayClockSnapshot();

    GameplayClockSnapshot(uint32_t audioTimeMs, uint32_t chartTimeMs,
                          bool chartClockDrivenByAudio, bool audioFinished
        );

    uint32_t getAudioTimeMs() const;

    uint32_t getChartTimeMs() const;

    bool isChartClockDrivenByAudio() const;

    bool isAudioFinished() const;

    void setAudioTimeMs(uint32_t value);

    void setChartTimeMs(uint32_t value);

    void setChartClockDrivenByAudio(bool value);

    void setAudioFinished(bool value);

private:
    uint32_t audioTimeMs_         = 0;
    uint32_t chartTimeMs_         = 0;
    bool chartClockDrivenByAudio_ = false;
    bool audioFinished_           = false;
};
