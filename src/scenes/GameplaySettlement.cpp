#include "GameplaySettlement.h"

#include "gameplay/GameplaySession.h"
#include "gameplay/GameplayRecordWriter.h"

bool GameplaySettlement::onEnterSettlement(const GameplaySession &session,
                                                   const std::string &chartID,
                                                   const std::string &songName,
                                                   const uint32_t timestampSec
    ) {
    if (saveTriggered_) return false;

    result_        = session.finalResult();
    hasResult_     = true;
    saveTriggered_ = true;
    saveSucceeded_ = GameplayRecordWriter::saveFinalResult(result_, chartID, songName, timestampSec);
    return saveSucceeded_;
}

bool GameplaySettlement::hasFinalResult() const {
    return hasResult_;
}

bool GameplaySettlement::recordSaveSucceeded() const {
    return saveSucceeded_;
}

GameplayFinalResult GameplaySettlement::finalResult() const {
    return result_;
}
