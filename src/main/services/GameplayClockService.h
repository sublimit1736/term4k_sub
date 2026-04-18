#pragma once

#include "entities/GameplayResult.h"

#include <cstdint>

class GameplayClockService {
public:
    void reset();

    void setChartClockDrivenByAudio(bool enabled);

    bool chartClockDrivenByAudio() const;

    void setAudioFinished(bool finished);

    bool audioFinished() const;

    uint32_t audioTimeMs() const;

    uint32_t chartTimeMs() const;

    void updateChartTime(uint32_t chartTimeMs);

    bool updateAudioTime(uint32_t audioTimeMs);

    GameplayClockSnapshot snapshot() const;

private:
    uint32_t audioTimeMs_         = 0;
    uint32_t chartTimeMs_         = 0;
    bool chartClockDrivenByAudio_ = false;
    bool audioFinished_           = false;
};
