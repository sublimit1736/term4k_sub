#pragma once

#include "entities/GameplayResult.h"
#include "services/GameplaySessionService.h"

#include <cstdint>
#include <string>

class GameplayInstance {
public:
    GameplayInstance();

    ~GameplayInstance();

    GameplayInstance(const GameplayInstance &) = delete;

    GameplayInstance &operator=(const GameplayInstance &) = delete;

    GameplayInstance(GameplayInstance &&) = delete;

    GameplayInstance &operator=(GameplayInstance &&) = delete;

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
    GameplaySessionService session_;
};
