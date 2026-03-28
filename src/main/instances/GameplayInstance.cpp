#include "GameplayInstance.h"

GameplayInstance::GameplayInstance() = default;

GameplayInstance::~GameplayInstance() = default;

bool GameplayInstance::openChart(const std::string &chartFilePath, const uint16_t keyCount) {
    return session_.openChart(chartFilePath, keyCount);
}

void GameplayInstance::reset() {
    session_.reset();
}

void GameplayInstance::advanceChartTimeMs(const uint32_t chartTimeMs) {
    session_.advanceChartTimeMs(chartTimeMs);
}

void GameplayInstance::advanceAudioTimeMs(const uint32_t audioTimeMs) {
    session_.advanceAudioTimeMs(audioTimeMs);
}

void GameplayInstance::setChartClockDrivenByAudio(const bool enabled) {
    session_.setChartClockDrivenByAudio(enabled);
}

void GameplayInstance::onKeyDown(const uint8_t keyCode) {
    session_.onKeyDown(keyCode);
}

void GameplayInstance::onKeyUp(const uint8_t keyCode) {
    session_.onKeyUp(keyCode);
}

void GameplayInstance::setAudioFinished(const bool finished) {
    session_.setAudioFinished(finished);
}

GameplayClockSnapshot GameplayInstance::clockSnapshot() const {
    return session_.clockSnapshot();
}

GameplaySnapshot GameplayInstance::snapshot() const {
    return session_.snapshot();
}

bool GameplayInstance::isResultReady() const {
    return session_.isResultReady();
}

GameplayFinalResult GameplayInstance::finalResult() const {
    return session_.finalResult();
}

uint32_t GameplayInstance::chartNoteCount() const {
    return session_.chartNoteCount();
}

uint64_t GameplayInstance::chartMaxScore() const {
    return session_.chartMaxScore();
}