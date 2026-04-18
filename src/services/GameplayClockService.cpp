#include "GameplayClockService.h"

void GameplayClockService::reset() {
    audioTimeMs_   = 0;
    chartTimeMs_   = 0;
    audioFinished_ = false;
}

void GameplayClockService::setChartClockDrivenByAudio(const bool enabled) {
    chartClockDrivenByAudio_ = enabled;
}

bool GameplayClockService::chartClockDrivenByAudio() const {
    return chartClockDrivenByAudio_;
}

void GameplayClockService::setAudioFinished(const bool finished) {
    audioFinished_ = finished;
}

bool GameplayClockService::audioFinished() const {
    return audioFinished_;
}

uint32_t GameplayClockService::audioTimeMs() const {
    return audioTimeMs_;
}

uint32_t GameplayClockService::chartTimeMs() const {
    return chartTimeMs_;
}

void GameplayClockService::updateChartTime(const uint32_t chartTimeMs) {
    chartTimeMs_ = chartTimeMs;
}

bool GameplayClockService::updateAudioTime(const uint32_t audioTimeMs) {
    audioTimeMs_ = audioTimeMs;
    if (!chartClockDrivenByAudio_) return false;
    chartTimeMs_ = audioTimeMs;
    return true;
}

GameplayClockSnapshot GameplayClockService::snapshot() const {
    GameplayClockSnapshot s;
    s.setAudioTimeMs(audioTimeMs_);
    s.setChartTimeMs(chartTimeMs_);
    s.setChartClockDrivenByAudio(chartClockDrivenByAudio_);
    s.setAudioFinished(audioFinished_);
    return s;
}
