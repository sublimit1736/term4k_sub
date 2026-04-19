#include "SettingsDraft.h"

SettingsDraft::SettingsDraft() = default;

SettingsDraft::SettingsDraft(std::string theme, std::string locale,
                             const float musicVolume, const float hitSoundVolume,
                             const uint32_t audioBufferSize, const bool showEarlyLate,
                             const bool showAPIndicator, const bool showFCIndicator,
                             const int32_t chartOffsetMs, const int32_t chartDisplayOffsetMs,
                             const uint32_t chartPreloadMs,
                             const ChartEndTimingMode chartEndTimingMode,
                             std::vector<uint8_t> keyBindings
    ) : theme_(std::move(theme)), locale_(std::move(locale)), musicVolume_(musicVolume),
        hitSoundVolume_(hitSoundVolume), audioBufferSize_(audioBufferSize),
        showEarlyLate_(showEarlyLate), showAPIndicator_(showAPIndicator),
        showFCIndicator_(showFCIndicator), chartOffsetMs_(chartOffsetMs),
        chartDisplayOffsetMs_(chartDisplayOffsetMs), chartPreloadMs_(chartPreloadMs),
        chartEndTimingMode_(chartEndTimingMode), keyBindings_(std::move(keyBindings)) {}

const std::string &SettingsDraft::getTheme() const { return theme_; }
const std::string &SettingsDraft::getLocale() const { return locale_; }
float SettingsDraft::getMusicVolume() const { return musicVolume_; }
float SettingsDraft::getHitSoundVolume() const { return hitSoundVolume_; }
uint32_t SettingsDraft::getAudioBufferSize() const { return audioBufferSize_; }
bool SettingsDraft::isShowEarlyLate() const { return showEarlyLate_; }
bool SettingsDraft::isShowAPIndicator() const { return showAPIndicator_; }
bool SettingsDraft::isShowFCIndicator() const { return showFCIndicator_; }
int32_t SettingsDraft::getChartOffsetMs() const { return chartOffsetMs_; }
int32_t SettingsDraft::getChartDisplayOffsetMs() const { return chartDisplayOffsetMs_; }
uint32_t SettingsDraft::getChartPreloadMs() const { return chartPreloadMs_; }
ChartEndTimingMode SettingsDraft::getChartEndTimingMode() const { return chartEndTimingMode_; }
const std::vector<uint8_t> &SettingsDraft::getKeyBindings() const { return keyBindings_; }

void SettingsDraft::setTheme(const std::string &value) { theme_ = value; }
void SettingsDraft::setLocale(const std::string &value) { locale_ = value; }
void SettingsDraft::setMusicVolume(const float value) { musicVolume_ = value; }
void SettingsDraft::setHitSoundVolume(const float value) { hitSoundVolume_ = value; }
void SettingsDraft::setAudioBufferSize(const uint32_t value) { audioBufferSize_ = value; }
void SettingsDraft::setShowEarlyLate(const bool value) { showEarlyLate_ = value; }
void SettingsDraft::setShowAPIndicator(const bool value) { showAPIndicator_ = value; }
void SettingsDraft::setShowFCIndicator(const bool value) { showFCIndicator_ = value; }
void SettingsDraft::setChartOffsetMs(const int32_t value) { chartOffsetMs_ = value; }
void SettingsDraft::setChartDisplayOffsetMs(const int32_t value) { chartDisplayOffsetMs_ = value; }
void SettingsDraft::setChartPreloadMs(const uint32_t value) { chartPreloadMs_ = value; }
void SettingsDraft::setChartEndTimingMode(const ChartEndTimingMode value) { chartEndTimingMode_ = value; }
void SettingsDraft::setKeyBindings(const std::vector<uint8_t> &value) { keyBindings_ = value; }
ToastPosition SettingsDraft::getToastPosition() const { return toastPosition_; }
void SettingsDraft::setToastPosition(const ToastPosition value) { toastPosition_ = value; }

bool SettingsDraft::isHudShowScore() const { return hudShowScore_; }
bool SettingsDraft::isHudShowAccuracy() const { return hudShowAccuracy_; }
bool SettingsDraft::isHudShowCombo() const { return hudShowCombo_; }
bool SettingsDraft::isHudShowMaxCombo() const { return hudShowMaxCombo_; }
bool SettingsDraft::isHudShowJudgements() const { return hudShowJudgements_; }
bool SettingsDraft::isHudShowProgress() const { return hudShowProgress_; }
bool SettingsDraft::isHudShowMaxAccCeiling() const { return hudShowMaxAccCeiling_; }
bool SettingsDraft::isHudShowPbDelta() const { return hudShowPbDelta_; }

void SettingsDraft::setHudShowScore(const bool value) { hudShowScore_ = value; }
void SettingsDraft::setHudShowAccuracy(const bool value) { hudShowAccuracy_ = value; }
void SettingsDraft::setHudShowCombo(const bool value) { hudShowCombo_ = value; }
void SettingsDraft::setHudShowMaxCombo(const bool value) { hudShowMaxCombo_ = value; }
void SettingsDraft::setHudShowJudgements(const bool value) { hudShowJudgements_ = value; }
void SettingsDraft::setHudShowProgress(const bool value) { hudShowProgress_ = value; }
void SettingsDraft::setHudShowMaxAccCeiling(const bool value) { hudShowMaxAccCeiling_ = value; }
void SettingsDraft::setHudShowPbDelta(const bool value) { hudShowPbDelta_ = value; }

bool SettingsDraft::operator==(const SettingsDraft &other) const {
    return theme_ == other.theme_ &&
           locale_ == other.locale_ &&
           musicVolume_ == other.musicVolume_ &&
           hitSoundVolume_ == other.hitSoundVolume_ &&
           audioBufferSize_ == other.audioBufferSize_ &&
           showEarlyLate_ == other.showEarlyLate_ &&
           showAPIndicator_ == other.showAPIndicator_ &&
           showFCIndicator_ == other.showFCIndicator_ &&
           chartOffsetMs_ == other.chartOffsetMs_ &&
           chartDisplayOffsetMs_ == other.chartDisplayOffsetMs_ &&
           chartPreloadMs_ == other.chartPreloadMs_ &&
           chartEndTimingMode_ == other.chartEndTimingMode_ &&
           keyBindings_ == other.keyBindings_ &&
           toastPosition_ == other.toastPosition_ &&
           hudShowScore_ == other.hudShowScore_ &&
           hudShowAccuracy_ == other.hudShowAccuracy_ &&
           hudShowCombo_ == other.hudShowCombo_ &&
           hudShowMaxCombo_ == other.hudShowMaxCombo_ &&
           hudShowJudgements_ == other.hudShowJudgements_ &&
           hudShowProgress_ == other.hudShowProgress_ &&
           hudShowMaxAccCeiling_ == other.hudShowMaxAccCeiling_ &&
           hudShowPbDelta_ == other.hudShowPbDelta_;
}

bool SettingsDraft::operator!=(const SettingsDraft &other) const {
    return !(*this == other);
}
