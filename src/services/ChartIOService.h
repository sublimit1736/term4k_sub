#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <utility>

// Chart event payload: event type + event data.
struct ChartEvent {
    uint8_t type;  // Event type code (click/push/release/changeBPM/changeTempo/changeStreamSpeed)
    uint32_t data; // Payload: 0 for tap/hold,
    //          bit-cast float for changeBPM/changeStreamSpeed,
    //          packed uint8 pair for changeTempo: (a << 8) | b.
};

// Chart buffer: timestamp -> event.
typedef std::map<uint32_t, ChartEvent> ChartBuffer;

// Note type ids in chart file format (uint8).
constexpr uint8_t tap                   = 0x01;
constexpr uint8_t hold                  = 0x02;
constexpr uint8_t changeBPMNote         = 0x03;
constexpr uint8_t changeTempoNote       = 0x04;
constexpr uint8_t changeStreamSpeedNote = 0x05;

// Event type ids used in ChartBuffer.
constexpr uint8_t click             = 1;
constexpr uint8_t push              = 2;
constexpr uint8_t release           = 3;
constexpr uint8_t changeBPM         = 4;
constexpr uint8_t changeTempo       = 5;
constexpr uint8_t changeStreamSpeed = 6;

// Chart file read/parse helper (all-static).
class ChartIOService {
public:
    // Reads a chart file and fills chartBuffer using chart metadata fields.
    static bool readChart(const char* fileName, ChartBuffer &chartBuffer,
                          uint16_t keyCount, float baseBPM,
                          std::pair<uint8_t, uint8_t> baseTempo
        );

    // Converts uint32 to an 8-char lowercase hex string.
    static std::string num2hex8str(uint32_t num);

    // Converts uint8 to a 2-char lowercase hex string.
    static std::string num2hex2str(uint8_t num);

private:
    // Hex lookup table.
    static constexpr char chex[17] = "0123456789abcdef";

    // Parses one hex character to numeric value.
    static constexpr uint8_t hexVal(char c) noexcept;

    // Parses 8 hex chars (uint32 field) from string offset.
    static uint32_t parseHexField(const std::string &str, int start);

    // Parses 2 hex chars (uint8 field) from string offset.
    static uint8_t parseHexField2(const std::string &str, int start);
};
