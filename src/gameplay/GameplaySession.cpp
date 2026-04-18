#include "GameplaySession.h"

#include "platform/RuntimeConfig.h"
#include "gameplay/GameplayChartParser.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace {
    constexpr int32_t kTapEarlyNoJudgeMs   = -150;
    constexpr int32_t kTapEarlyMissMs      = -100;
    constexpr int32_t kTapGreatWindowMs    = 100;
    constexpr int32_t kTapPerfectWindowMs  = 50;
    constexpr int32_t kTapEarlyLateSplitMs = 25;
    constexpr int32_t kTapLateAutoMissMs   = 150;

    constexpr int32_t kHoldHeadWindowMs    = 100;
    constexpr int32_t kHoldTailEarlyMissMs = -150;
    constexpr uint32_t kSettlementDelayMs  = 3000;
}

bool GameplaySession::openChart(const std::string &chartFilePath, const uint16_t keyCount) {
    reset();

    if (keyCount == 0) return false;
    lanes_.resize(keyCount);

    for (uint8_t lane = 0; lane < keyCount && lane < RuntimeConfig::keyBindings.size(); ++lane){
        const uint8_t key = RuntimeConfig::keyBindings[lane];
        if (!keyToLane_.contains(key)){
            keyToLane_[key] = lane;
        }
    }

    GameplayChartData chartData;
    if (!GameplayChartParser::parseChart(chartFilePath, keyCount, chartData)) return false;

    taps_.reserve(chartData.getTaps().size());
    for (const auto &tap: chartData.getTaps()){
        taps_.push_back(TapNote{tap.getLane(), tap.getTimeMs(), false});
    }

    holds_.reserve(chartData.getHolds().size());
    for (const auto &hold: chartData.getHolds()){
        holds_.push_back(HoldNote{hold.getLane(), hold.getHeadTimeMs(), hold.getTailTimeMs(), false, false});
    }

    chartEndTimeMs_ = chartData.getEndTimeMs();
    stats_.reset(chartData.getNoteCount(), chartData.getMaxScore());
    rebuildLaneOrders();
    loadWindowAllLanes();
    return true;
}

void GameplaySession::reset() {
    taps_.clear();
    holds_.clear();
    lanes_.clear();
    keyToLane_.clear();

    clock_.reset();
    stats_.reset(0, 0);

    settlementAnimationTriggered_ = false;
    resultReady_                  = false;
    chartEndTimeMs_               = 0;
}

void GameplaySession::advanceChartTimeMs(const uint32_t chartTimeMs) {
    clock_.updateChartTime(chartTimeMs);
    loadWindowAllLanes();

    for (std::size_t lane = 0; lane < lanes_.size(); ++lane){
        autoJudgeByTime(static_cast<uint8_t>(lane));
    }

    settleIfReady();
}

void GameplaySession::advanceAudioTimeMs(const uint32_t audioTimeMs) {
    if (clock_.updateAudioTime(audioTimeMs)){
        advanceChartTimeMs(clock_.chartTimeMs());
    }
}

void GameplaySession::setChartClockDrivenByAudio(const bool enabled) {
    clock_.setChartClockDrivenByAudio(enabled);
}

void GameplaySession::onKeyDown(const uint8_t keyCode) {
    const auto laneOpt = laneByKey(keyCode);
    if (!laneOpt.has_value()) return;

    const uint8_t lane = laneOpt.value();
    auto &state        = lanes_[lane];
    if (state.isPressed) return;
    state.isPressed = true;

    loadWindowForLane(lane);
    cleanupResolvedFront(lane);

    if (state.pendingTapPos < state.tapLoadPos){
        const std::size_t tapIdx = state.tapOrder[state.pendingTapPos];
        auto &tap                = taps_[tapIdx];
        if (!tap.resolved){
            const int32_t delta = static_cast<int32_t>(clock_.chartTimeMs()) - static_cast<int32_t>(tap.timeMs);
            if (delta >= kTapEarlyNoJudgeMs){
                settleTapByDelta(tapIdx, delta);
                ++state.pendingTapPos;
                cleanupResolvedFront(lane);
                return;
            }
            return;
        }
        ++state.pendingTapPos;
    }

    cleanupResolvedFront(lane);

    if (state.pendingHoldPos >= state.holdLoadPos) return;
    const std::size_t holdIdx = state.holdOrder[state.pendingHoldPos];
    auto &hold                = holds_[holdIdx];
    if (hold.resolved || hold.headPerfect){
        ++state.pendingHoldPos;
        cleanupResolvedFront(lane);
        return;
    }

    const int32_t delta = static_cast<int32_t>(clock_.chartTimeMs()) - static_cast<int32_t>(hold.headTimeMs);
    if (delta < -kHoldHeadWindowMs) return;

    if (delta <= kHoldHeadWindowMs){
        hold.headPerfect = true;
        state.activeHoldIndices.push_back(holdIdx);
        ++state.pendingHoldPos;
        return;
    }

    settleHoldHeadMiss(holdIdx);
    ++state.pendingHoldPos;
    cleanupResolvedFront(lane);
}

void GameplaySession::onKeyUp(const uint8_t keyCode) {
    const auto laneOpt = laneByKey(keyCode);
    if (!laneOpt.has_value()) return;

    const uint8_t lane = laneOpt.value();
    auto &state        = lanes_[lane];
    if (!state.isPressed) return;
    state.isPressed = false;

    if (state.activeHoldIndices.empty()) return;

    auto earliestIt       = state.activeHoldIndices.end();
    uint32_t earliestTail = std::numeric_limits<uint32_t>::max();
    for (auto it = state.activeHoldIndices.begin(); it != state.activeHoldIndices.end(); ++it){
        const std::size_t idx = *it;
        const auto &hold      = holds_[idx];
        if (hold.resolved || !hold.headPerfect) continue;
        if (hold.tailTimeMs < earliestTail){
            earliestTail = hold.tailTimeMs;
            earliestIt   = it;
        }
    }

    if (earliestIt == state.activeHoldIndices.end()) return;

    const std::size_t holdIdx = *earliestIt;
    const int32_t delta = static_cast<int32_t>(clock_.chartTimeMs()) - static_cast<int32_t>(holds_[holdIdx].tailTimeMs);

    if (delta < kHoldTailEarlyMissMs){
        settleHoldTail(holdIdx, GameplayJudgement::Miss);
    }
    else{
        settleHoldTail(holdIdx, GameplayJudgement::Perfect);
    }

    state.activeHoldIndices.erase(earliestIt);
}

void GameplaySession::setAudioFinished(const bool finished) {
    clock_.setAudioFinished(finished);
}

GameplayClockSnapshot GameplaySession::clockSnapshot() const {
    return clock_.snapshot();
}

GameplaySnapshot GameplaySession::snapshot() const {
    return stats_.buildSnapshot(clock_.chartTimeMs(), settlementAnimationTriggered_, resultReady_);
}

bool GameplaySession::isResultReady() const {
    return resultReady_;
}

GameplayFinalResult GameplaySession::finalResult() const {
    return stats_.buildFinalResult();
}

uint32_t GameplaySession::chartNoteCount() const {
    return stats_.chartNoteCount();
}

uint64_t GameplaySession::chartMaxScore() const {
    return stats_.chartMaxScore();
}

std::optional<uint8_t> GameplaySession::laneByKey(const uint8_t keyCode) const {
    const auto it = keyToLane_.find(keyCode);
    if (it == keyToLane_.end()) return std::nullopt;
    return it->second;
}

void GameplaySession::loadWindowForLane(const uint8_t lane) {
    if (lane >= lanes_.size()) return;
    auto &state              = lanes_[lane];
    const uint64_t windowEnd = static_cast<uint64_t>(clock_.chartTimeMs()) + RuntimeConfig::chartPreloadMs;

    while (state.tapLoadPos < state.tapOrder.size()){
        const std::size_t noteIdx = state.tapOrder[state.tapLoadPos];
        if (static_cast<uint64_t>(taps_[noteIdx].timeMs) > windowEnd) break;
        ++state.tapLoadPos;
    }

    while (state.holdLoadPos < state.holdOrder.size()){
        const std::size_t noteIdx = state.holdOrder[state.holdLoadPos];
        if (static_cast<uint64_t>(holds_[noteIdx].headTimeMs) > windowEnd) break;
        ++state.holdLoadPos;
    }
}

void GameplaySession::loadWindowAllLanes() {
    for (std::size_t lane = 0; lane < lanes_.size(); ++lane){
        loadWindowForLane(static_cast<uint8_t>(lane));
    }
}

void GameplaySession::settleTapByDelta(const std::size_t tapIndex, const int32_t deltaMs) {
    auto &tap = taps_[tapIndex];
    if (tap.resolved) return;

    const bool isEarly = deltaMs < -kTapEarlyLateSplitMs;
    const bool isLate  = deltaMs > kTapEarlyLateSplitMs;

    if ((deltaMs >= kTapEarlyNoJudgeMs && deltaMs <= kTapEarlyMissMs) || deltaMs > kTapGreatWindowMs){
        stats_.settleNote(GameplayJudgement::Miss, isEarly, isLate);
    }
    else if (deltaMs >= -kTapPerfectWindowMs && deltaMs <= kTapPerfectWindowMs){
        stats_.settleNote(GameplayJudgement::Perfect, isEarly, isLate);
    }
    else{
        stats_.settleNote(GameplayJudgement::Great, isEarly, isLate);
    }

    tap.resolved = true;
}

void GameplaySession::settleHoldHeadMiss(const std::size_t holdIndex) {
    auto &hold = holds_[holdIndex];
    if (hold.resolved) return;
    hold.resolved    = true;
    hold.headPerfect = false;
    stats_.settleNote(GameplayJudgement::Miss, false, false);
}

void GameplaySession::settleHoldTail(const std::size_t holdIndex, const GameplayJudgement judgement) {
    auto &hold = holds_[holdIndex];
    if (hold.resolved) return;
    hold.resolved    = true;
    hold.headPerfect = (judgement == GameplayJudgement::Perfect);
    stats_.settleNote(judgement, false, false);
}

void GameplaySession::cleanupResolvedFront(const uint8_t lane) {
    auto &state = lanes_[lane];

    while (state.pendingTapPos < state.tapLoadPos){
        const std::size_t idx = state.tapOrder[state.pendingTapPos];
        if (!taps_[idx].resolved) break;
        ++state.pendingTapPos;
    }

    while (state.pendingHoldPos < state.holdLoadPos){
        const std::size_t idx = state.holdOrder[state.pendingHoldPos];
        if (!holds_[idx].resolved) break;
        ++state.pendingHoldPos;
    }

    std::erase_if(state.activeHoldIndices,
                  [&](const std::size_t idx) { return holds_[idx].resolved; });
}

void GameplaySession::autoJudgeByTime(const uint8_t lane) {
    auto &state = lanes_[lane];

    while (state.pendingTapPos < state.tapLoadPos){
        const std::size_t tapIdx = state.tapOrder[state.pendingTapPos];
        auto &tap                = taps_[tapIdx];
        if (tap.resolved){
            ++state.pendingTapPos;
            continue;
        }
        if (clock_.chartTimeMs() <= tap.timeMs + static_cast<uint32_t>(kTapLateAutoMissMs)) break;
        settleTapByDelta(tapIdx, static_cast<int32_t>(clock_.chartTimeMs()) - static_cast<int32_t>(tap.timeMs));
        ++state.pendingTapPos;
    }

    while (state.pendingHoldPos < state.holdLoadPos){
        const std::size_t holdIdx = state.holdOrder[state.pendingHoldPos];
        auto &hold                = holds_[holdIdx];
        if (hold.resolved || hold.headPerfect){
            ++state.pendingHoldPos;
            continue;
        }
        if (clock_.chartTimeMs() <= hold.headTimeMs + static_cast<uint32_t>(kHoldHeadWindowMs)) break;
        settleHoldHeadMiss(holdIdx);
        ++state.pendingHoldPos;
    }

    if (!state.activeHoldIndices.empty() && state.isPressed){
        for (const std::size_t holdIdx: state.activeHoldIndices){
            auto &hold = holds_[holdIdx];
            if (hold.resolved || !hold.headPerfect) continue;
            if (clock_.chartTimeMs() >= hold.tailTimeMs){
                settleHoldTail(holdIdx, GameplayJudgement::Perfect);
            }
        }
    }

    cleanupResolvedFront(lane);
}

void GameplaySession::settleIfReady() {
    if (resultReady_) return;
    if (clock_.chartTimeMs() < chartEndTimeMs_ + kSettlementDelayMs) return;

    settlementAnimationTriggered_ = true;
    resultReady_                  = true;
}

void GameplaySession::rebuildLaneOrders() {
    for (auto &lane: lanes_){
        lane.tapOrder.clear();
        lane.holdOrder.clear();
        lane.tapLoadPos     = 0;
        lane.holdLoadPos    = 0;
        lane.pendingTapPos  = 0;
        lane.pendingHoldPos = 0;
        lane.activeHoldIndices.clear();
        lane.isPressed = false;
    }

    for (std::size_t i = 0; i < taps_.size(); ++i){
        lanes_[taps_[i].lane].tapOrder.push_back(i);
    }
    for (std::size_t i = 0; i < holds_.size(); ++i){
        lanes_[holds_[i].lane].holdOrder.push_back(i);
    }

    for (auto &lane: lanes_){
        std::ranges::sort(lane.tapOrder, [&](const std::size_t lhs, const std::size_t rhs) {
            return taps_[lhs].timeMs<taps_[rhs].timeMs;
        });
        std::ranges::sort(lane.holdOrder, [&](const std::size_t lhs, const std::size_t rhs) {
            return holds_[lhs].headTimeMs<holds_[rhs].headTimeMs;
        });
    }
}
