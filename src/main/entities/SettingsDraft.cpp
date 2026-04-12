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
           keyBindings_ == other.keyBindings_;
}

bool SettingsDraft::operator!=(const SettingsDraft &other) const {
    return !(*this == other);
}

