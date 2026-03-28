#include "ChartIOService.h"
#include "I18nService.h"
#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "utils/ErrorNotifier.h"

#include <fstream>
#include <limits>

namespace {
    uint32_t applyTimingConfig(uint32_t rawTime) {
        constexpr int64_t kLegacyDefaultPreloadMs = 2000;
        // Keep legacy timing behavior when preload=2000; user settings apply as delta.
        const int64_t shift = static_cast<int64_t>(RuntimeConfigs::chartOffsetMs) +
                              static_cast<int64_t>(RuntimeConfigs::chartPreloadMs) -
                              kLegacyDefaultPreloadMs;
        const int64_t adjusted = static_cast<int64_t>(rawTime) + shift;

        int64_t clamped = adjusted;
        if (clamped < 0) clamped = 0;
        if (clamped > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())){
            clamped = static_cast<int64_t>(std::numeric_limits<uint32_t>::max());
        }
        return static_cast<uint32_t>(clamped);
    }

    bool getMappedKey(uint8_t track, uint8_t &mappedKey) {
        if (track >= RuntimeConfigs::keyBindings.size()) return false;
        mappedKey = RuntimeConfigs::keyBindings[track];
        return true;
    }

    std::string resolveChartPath(const char* fileName) {
        if (fileName == nullptr || fileName[0] == '\0') return "";

        const std::string input(fileName);
        if (!input.empty() && input.front() == '/') return input; // Already an absolute path

        AppDirs::init();
        return AppDirs::chartsDir() + input;
    }
} // namespace

// Parses one hex character to value (out-of-class definition).
constexpr uint8_t ChartIOService::hexVal(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0;
}

// Reads chart file and fills the event buffer.
// Per-line formats are based on note type (first 2 hex chars):
//   tap              : type(2h) + track(2h) + t1(8h)               = 12 chars
//   hold             : type(2h) + track(2h) + t1(8h) + t2(8h)      = 20 chars
//   changeBPM        : type(2h) + float(8h) + timestamp(8h)        = 18 chars
//   changeTempo      : type(2h) + a(2h)   + b(2h)   + timestamp(8h)= 14 chars
//   changeStreamSpeed: type(2h) + float(8h) + timestamp(8h)        = 18 chars
bool ChartIOService::readChart(const char* fileName, ChartBuffer &chartBuffer,
                               uint16_t keyCount, float /*base BPM*/,
                               std::pair<uint8_t, uint8_t> /*base tempo*/
    ) {
    const std::string resolvedPath = resolveChartPath(fileName);
    std::ifstream chartFile(resolvedPath, std::ios::in);

    if (!chartFile){
        ErrorNotifier::notify(
                              I18nService::instance().get("error.chart_open_failed") + ": " +
                              (resolvedPath.empty() ? I18nService::instance().get("error.empty_path") : resolvedPath));
        return false;
    }

    std::string chartSignBegin;
    std::getline(chartFile, chartSignBegin);

    if (chartSignBegin == "t4kcb"){
        std::string itemHandle;

        while (std::getline(chartFile, itemHandle)){
            // Stop at end marker.
            if (itemHandle == "t4kce") break;

            // Need at least 2 chars to parse note type.
            if (itemHandle.size() < 2) continue;

            uint8_t itemType = parseHexField2(itemHandle, 0);

            if (itemType == tap){
                // tap: type(2h) + track(2h) + t1(8h) = 12 chars
                if (itemHandle.size() < 12) continue;
                uint8_t track = parseHexField2(itemHandle, 2);
                if (track >= keyCount){
                    ErrorNotifier::notify(
                                          I18nService::instance().get("error.tap_track_out_of_range") +
                                          " (track=" + std::to_string(static_cast<int>(track)) +
                                          ", keyCount=" + std::to_string(keyCount) + ")");
                    continue;
                }
                uint32_t t1            = applyTimingConfig(parseHexField(itemHandle, 4));
                uint8_t mappedKey      = 0;
                const uint32_t payload = getMappedKey(track, mappedKey)
                                             ? static_cast<uint32_t>(mappedKey)
                                             : 0u;
                chartBuffer[t1] = ChartEvent{click, payload};
            }
            else if (itemType == hold){
                // hold: type(2h) + track(2h) + t1(8h) + t2(8h) = 20 chars
                if (itemHandle.size() < 20) continue;
                uint8_t track = parseHexField2(itemHandle, 2);
                if (track >= keyCount){
                    ErrorNotifier::notify(
                                          I18nService::instance().get("error.hold_track_out_of_range") +
                                          " (track=" + std::to_string(static_cast<int>(track)) +
                                          ", keyCount=" + std::to_string(keyCount) + ")");
                    continue;
                }
                uint32_t t1            = applyTimingConfig(parseHexField(itemHandle, 4));
                uint32_t t2            = applyTimingConfig(parseHexField(itemHandle, 12));
                uint8_t mappedKey      = 0;
                const uint32_t payload = getMappedKey(track, mappedKey)
                                             ? static_cast<uint32_t>(mappedKey)
                                             : 0u;
                chartBuffer[t1] = ChartEvent{push, payload};
                chartBuffer[t2] = ChartEvent{release, payload};
            }
            else if (itemType == changeBPMNote){
                // changeBPM: type(2h) + float(8h) + timestamp(8h) = 18 chars
                if (itemHandle.size() < 18) continue;
                uint32_t floatBits = parseHexField(itemHandle, 2);
                uint32_t t         = applyTimingConfig(parseHexField(itemHandle, 10));
                chartBuffer[t]     = ChartEvent{changeBPM, floatBits};
            }
            else if (itemType == changeTempoNote){
                // changeTempo: type(2h) + a(2h) + b(2h) + timestamp(8h) = 14 chars
                if (itemHandle.size() < 14) continue;
                uint8_t a       = parseHexField2(itemHandle, 2);
                uint8_t b       = parseHexField2(itemHandle, 4);
                uint32_t t      = applyTimingConfig(parseHexField(itemHandle, 6));
                uint32_t packed = (static_cast<uint32_t>(a) << 8) | b;
                chartBuffer[t]  = ChartEvent{changeTempo, packed};
            }
            else if (itemType == changeStreamSpeedNote){
                // changeStreamSpeed: type(2h) + float(8h) + timestamp(8h) = 18 chars
                if (itemHandle.size() < 18) continue;
                uint32_t floatBits = parseHexField(itemHandle, 2);
                uint32_t t         = applyTimingConfig(parseHexField(itemHandle, 10));
                chartBuffer[t]     = ChartEvent{changeStreamSpeed, floatBits};
            }
        }
        return true;
    }
    ErrorNotifier::notify(I18nService::instance().get("error.chart_invalid_format"));
    return false;
}

// Converts uint32 to an 8-char hex string.
std::string ChartIOService::num2hex8str(uint32_t num) {
    std::string s(8, '0');
    for (int i = 7; i >= 0; --i){
        s[i] = chex[num & 0xF];
        num  >>= 4;
    }
    return s;
}

// Converts uint8 to a 2-char hex string.
std::string ChartIOService::num2hex2str(uint8_t num) {
    std::string s(2, '0');
    s[0] = chex[(num >> 4) & 0xF];
    s[1] = chex[num & 0xF];
    return s;
}

// Parses 8 hex chars from string offset into uint32.
uint32_t ChartIOService::parseHexField(const std::string &str, int start) {
    uint32_t val = 0;
    for (int i = start; i < start + 8; ++i){
        val = hexVal(str[i]) | (val << 4);
    }
    return val;
}

// Parses 2 hex chars from string offset into uint8.
uint8_t ChartIOService::parseHexField2(const std::string &str, int start) {
    uint8_t val = 0;
    for (int i = start; i < start + 2; ++i){
        val = static_cast<uint8_t>((val << 4) | hexVal(str[i]));
    }
    return val;
}