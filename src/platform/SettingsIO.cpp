#include "SettingsIO.h"

#include "platform/RuntimeConfig.h"

SettingsDraft SettingsIO::snapshotFromRuntime() {
    SettingsDraft draft;
    draft.setTheme(RuntimeConfig::theme);
    draft.setLocale(RuntimeConfig::locale);

    draft.setMusicVolume(RuntimeConfig::musicVolume);
    draft.setHitSoundVolume(RuntimeConfig::hitSoundVolume);
    draft.setAudioBufferSize(RuntimeConfig::audioBufferSize);

    draft.setShowEarlyLate(RuntimeConfig::showEarlyLate);
    draft.setShowAPIndicator(RuntimeConfig::showAPIndicator);
    draft.setShowFCIndicator(RuntimeConfig::showFCIndicator);
    draft.setChartOffsetMs(RuntimeConfig::chartOffsetMs);
    draft.setChartDisplayOffsetMs(RuntimeConfig::chartDisplayOffsetMs);
    draft.setChartPreloadMs(RuntimeConfig::chartPreloadMs);
    draft.setChartEndTimingMode(RuntimeConfig::chartEndTimingMode);
    draft.setKeyBindings(RuntimeConfig::keyBindings);
    draft.setToastPosition(RuntimeConfig::toastPosition);
    draft.setHudShowScore(RuntimeConfig::hudShowScore);
    draft.setHudShowAccuracy(RuntimeConfig::hudShowAccuracy);
    draft.setHudShowCombo(RuntimeConfig::hudShowCombo);
    draft.setHudShowMaxCombo(RuntimeConfig::hudShowMaxCombo);
    draft.setHudShowJudgements(RuntimeConfig::hudShowJudgements);
    draft.setHudShowProgress(RuntimeConfig::hudShowProgress);
    draft.setHudShowMaxAccCeiling(RuntimeConfig::hudShowMaxAccCeiling);
    draft.setHudShowPbDelta(RuntimeConfig::hudShowPbDelta);
    return draft;
}

void SettingsIO::applyToRuntime(const SettingsDraft &draft) {
    RuntimeConfig::theme  = draft.getTheme();
    RuntimeConfig::locale = draft.getLocale();

    RuntimeConfig::musicVolume     = draft.getMusicVolume();
    RuntimeConfig::hitSoundVolume  = draft.getHitSoundVolume();
    RuntimeConfig::audioBufferSize = draft.getAudioBufferSize();

    RuntimeConfig::showEarlyLate        = draft.isShowEarlyLate();
    RuntimeConfig::showAPIndicator      = draft.isShowAPIndicator();
    RuntimeConfig::showFCIndicator      = draft.isShowFCIndicator();
    RuntimeConfig::chartOffsetMs        = draft.getChartOffsetMs();
    RuntimeConfig::chartDisplayOffsetMs = draft.getChartDisplayOffsetMs();
    RuntimeConfig::chartPreloadMs       = draft.getChartPreloadMs();
    RuntimeConfig::chartEndTimingMode   = draft.getChartEndTimingMode();
    RuntimeConfig::keyBindings          = draft.getKeyBindings();
    RuntimeConfig::toastPosition        = draft.getToastPosition();
    RuntimeConfig::hudShowScore         = draft.isHudShowScore();
    RuntimeConfig::hudShowAccuracy      = draft.isHudShowAccuracy();
    RuntimeConfig::hudShowCombo         = draft.isHudShowCombo();
    RuntimeConfig::hudShowMaxCombo      = draft.isHudShowMaxCombo();
    RuntimeConfig::hudShowJudgements    = draft.isHudShowJudgements();
    RuntimeConfig::hudShowProgress      = draft.isHudShowProgress();
    RuntimeConfig::hudShowMaxAccCeiling = draft.isHudShowMaxAccCeiling();
    RuntimeConfig::hudShowPbDelta       = draft.isHudShowPbDelta();
}

bool SettingsIO::saveDraftForUser(const SettingsDraft &draft, const std::string &username) {
    const SettingsDraft before = snapshotFromRuntime();
    applyToRuntime(draft);
    if (RuntimeConfig::saveForUser(username)) return true;

    applyToRuntime(before);
    return false;
}
