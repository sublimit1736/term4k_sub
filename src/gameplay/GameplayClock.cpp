#include "GameplayClock.h"

void GameplayClock::reset() {
    audioTimeMs_   = 0;
    chartTimeMs_   = 0;
    audioFinished_ = false;
}

void GameplayClock::setChartClockDrivenByAudio(const bool enabled) {
    chartClockDrivenByAudio_ = enabled;
}

bool GameplayClock::chartClockDrivenByAudio() const {
    return chartClockDrivenByAudio_;
}

void GameplayClock::setAudioFinished(const bool finished) {
    audioFinished_ = finished;
}

bool GameplayClock::audioFinished() const {
    return audioFinished_;
}

uint32_t GameplayClock::audioTimeMs() const {
    return audioTimeMs_;
}

uint32_t GameplayClock::chartTimeMs() const {
    return chartTimeMs_;
}

void GameplayClock::updateChartTime(const uint32_t chartTimeMs) {
    chartTimeMs_ = chartTimeMs;
}

bool GameplayClock::updateAudioTime(const uint32_t audioTimeMs) {
    audioTimeMs_ = audioTimeMs;
    if (!chartClockDrivenByAudio_) return false;
    chartTimeMs_ = audioTimeMs;
    return true;
}

GameplayClockSnapshot GameplayClock::snapshot() const {
    GameplayClockSnapshot s;
    s.setAudioTimeMs(audioTimeMs_);
    s.setChartTimeMs(chartTimeMs_);
    s.setChartClockDrivenByAudio(chartClockDrivenByAudio_);
    s.setAudioFinished(audioFinished_);
    return s;
}
