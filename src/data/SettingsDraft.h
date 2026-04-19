#pragma once

#include "platform/RuntimeConfig.h"

#include <cstdint>
#include <string>
#include <vector>

class SettingsDraft {
public:
    SettingsDraft();

    SettingsDraft(std::string theme, std::string locale,
                  float musicVolume, float hitSoundVolume, uint32_t audioBufferSize,
                  bool showEarlyLate, bool showAPIndicator, bool showFCIndicator,
                  int32_t chartOffsetMs, int32_t chartDisplayOffsetMs, uint32_t chartPreloadMs,
                  ChartEndTimingMode chartEndTimingMode, std::vector<uint8_t> keyBindings
        );

    const std::string &getTheme() const;

    const std::string &getLocale() const;

    float getMusicVolume() const;

    float getHitSoundVolume() const;

    uint32_t getAudioBufferSize() const;

    bool isShowEarlyLate() const;

    bool isShowAPIndicator() const;

    bool isShowFCIndicator() const;

    int32_t getChartOffsetMs() const;

    int32_t getChartDisplayOffsetMs() const;

    uint32_t getChartPreloadMs() const;

    ChartEndTimingMode getChartEndTimingMode() const;

    const std::vector<uint8_t> &getKeyBindings() const;

    ToastPosition getToastPosition() const;

    bool isHudShowScore() const;
    bool isHudShowAccuracy() const;
    bool isHudShowCombo() const;
    bool isHudShowMaxCombo() const;
    bool isHudShowJudgements() const;
    bool isHudShowProgress() const;
    bool isHudShowMaxAccCeiling() const;
    bool isHudShowPbDelta() const;

    void setTheme(const std::string &value);

    void setLocale(const std::string &value);

    void setMusicVolume(float value);

    void setHitSoundVolume(float value);

    void setAudioBufferSize(uint32_t value);

    void setShowEarlyLate(bool value);

    void setShowAPIndicator(bool value);

    void setShowFCIndicator(bool value);

    void setChartOffsetMs(int32_t value);

    void setChartDisplayOffsetMs(int32_t value);

    void setChartPreloadMs(uint32_t value);

    void setChartEndTimingMode(ChartEndTimingMode value);

    void setKeyBindings(const std::vector<uint8_t> &value);

    void setToastPosition(ToastPosition value);

    void setHudShowScore(bool value);
    void setHudShowAccuracy(bool value);
    void setHudShowCombo(bool value);
    void setHudShowMaxCombo(bool value);
    void setHudShowJudgements(bool value);
    void setHudShowProgress(bool value);
    void setHudShowMaxAccCeiling(bool value);
    void setHudShowPbDelta(bool value);

    bool operator==(const SettingsDraft &other) const;

    bool operator!=(const SettingsDraft &other) const;

private:
    std::string theme_;
    std::string locale_;

    float musicVolume_        = 1.0f;
    float hitSoundVolume_     = 1.0f;
    uint32_t audioBufferSize_ = 512;

    bool showEarlyLate_                    = true;
    bool showAPIndicator_                  = true;
    bool showFCIndicator_                  = true;
    int32_t chartOffsetMs_                 = 0;
    int32_t chartDisplayOffsetMs_          = 0;
    uint32_t chartPreloadMs_               = 2000;
    ChartEndTimingMode chartEndTimingMode_ = ChartEndTimingMode::AfterChartEnd;
    std::vector<uint8_t> keyBindings_;
    ToastPosition toastPosition_           = ToastPosition::TopRight;

    bool hudShowScore_         = true;
    bool hudShowAccuracy_      = true;
    bool hudShowCombo_         = true;
    bool hudShowMaxCombo_      = false;
    bool hudShowJudgements_    = false;
    bool hudShowProgress_      = false;
    bool hudShowMaxAccCeiling_ = false;
    bool hudShowPbDelta_       = false;
};
