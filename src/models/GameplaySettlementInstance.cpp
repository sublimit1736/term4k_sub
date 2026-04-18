#include "GameplaySettlementInstance.h"

#include "services/GameplaySessionService.h"
#include "services/GameplayRecordService.h"

bool GameplaySettlementInstance::onEnterSettlement(const GameplaySessionService &session,
                                                   const std::string &chartID,
                                                   const std::string &songName,
                                                   const uint32_t timestampSec
    ) {
    if (saveTriggered_) return false;

    result_        = session.finalResult();
    hasResult_     = true;
    saveTriggered_ = true;
    saveSucceeded_ = GameplayRecordService::saveFinalResult(result_, chartID, songName, timestampSec);
    return saveSucceeded_;
}

bool GameplaySettlementInstance::hasFinalResult() const {
    return hasResult_;
}

bool GameplaySettlementInstance::recordSaveSucceeded() const {
    return saveSucceeded_;
}

GameplayFinalResult GameplaySettlementInstance::finalResult() const {
    return result_;
}
