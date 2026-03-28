#include "SettingsService.h"

#include "config/RuntimeConfigs.h"

SettingsDraft SettingsService::snapshotFromRuntime() {
    SettingsDraft draft;
    draft.setTheme(RuntimeConfigs::theme);
    draft.setLocale(RuntimeConfigs::locale);

    draft.setMusicVolume(RuntimeConfigs::musicVolume);
    draft.setHitSoundVolume(RuntimeConfigs::hitSoundVolume);
    draft.setAudioBufferSize(RuntimeConfigs::audioBufferSize);

    draft.setShowEarlyLate(RuntimeConfigs::showEarlyLate);
    draft.setShowAPIndicator(RuntimeConfigs::showAPIndicator);
    draft.setShowFCIndicator(RuntimeConfigs::showFCIndicator);
    draft.setChartOffsetMs(RuntimeConfigs::chartOffsetMs);
    draft.setChartDisplayOffsetMs(RuntimeConfigs::chartDisplayOffsetMs);
    draft.setChartPreloadMs(RuntimeConfigs::chartPreloadMs);
    draft.setChartEndTimingMode(RuntimeConfigs::chartEndTimingMode);
    draft.setKeyBindings(RuntimeConfigs::keyBindings);
    return draft;
}

void SettingsService::applyToRuntime(const SettingsDraft &draft) {
    RuntimeConfigs::theme  = draft.getTheme();
    RuntimeConfigs::locale = draft.getLocale();

    RuntimeConfigs::musicVolume     = draft.getMusicVolume();
    RuntimeConfigs::hitSoundVolume  = draft.getHitSoundVolume();
    RuntimeConfigs::audioBufferSize = draft.getAudioBufferSize();

    RuntimeConfigs::showEarlyLate        = draft.isShowEarlyLate();
    RuntimeConfigs::showAPIndicator      = draft.isShowAPIndicator();
    RuntimeConfigs::showFCIndicator      = draft.isShowFCIndicator();
    RuntimeConfigs::chartOffsetMs        = draft.getChartOffsetMs();
    RuntimeConfigs::chartDisplayOffsetMs = draft.getChartDisplayOffsetMs();
    RuntimeConfigs::chartPreloadMs       = draft.getChartPreloadMs();
    RuntimeConfigs::chartEndTimingMode   = draft.getChartEndTimingMode();
    RuntimeConfigs::keyBindings          = draft.getKeyBindings();
}

bool SettingsService::saveDraftForUser(const SettingsDraft &draft, const std::string &username) {
    const SettingsDraft before = snapshotFromRuntime();
    applyToRuntime(draft);
    if (RuntimeConfigs::saveForUser(username)) return true;

    applyToRuntime(before);
    return false;
}


