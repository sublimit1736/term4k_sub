#pragma once

#include <string>
#include <utility>
#include <cstdint>

// Chart metadata entity.
class Chart {
public:
    // Default constructor.
    Chart();

    // Full-argument constructor.
    Chart(std::string i, std::string d, std::string a, std::string c, std::string b, const float df,
          uint32_t pb                    = 0, uint32_t pe = 0, uint16_t kc = 0, float bb = 0.0f,
          std::pair<uint8_t, uint8_t> bt = {0, 0}
        );

    // --- Getters ---
    std::string getID() const;

    std::string getDisplayName() const;

    std::string getArtist() const;

    std::string getCharter() const;

    std::string getBPM() const;

    float getDifficulty() const;

    uint32_t getPreviewBegin() const;

    uint32_t getPreviewEnd() const;

    uint16_t getKeyCount() const;

    float getBaseBPM() const;

    std::pair<uint8_t, uint8_t> getBaseTempo() const;

    // --- Setters ---
    void setID(const std::string &value);

    void setDisplayName(const std::string &value);

    void setArtist(const std::string &value);

    void setCharter(const std::string &value);

    void setBPM(const std::string &value);

    void setDifficulty(float value);

    void setPreviewBegin(uint32_t value);

    void setPreviewEnd(uint32_t value);

    void setKeyCount(uint16_t value);

    void setBaseBPM(float value);

    void setBaseTempo(std::pair<uint8_t, uint8_t> value);

    // Serializes metadata to string.
    std::string serializeString() const;

    // Deserializes from string (instance method).
    void deserializeFromString(const std::string &s);

    // Deserializes from string (static factory method).
    static Chart deserializeString(const std::string &s);

private:
    std::string id;
    std::string displayName;
    std::string artist;
    std::string charter;
    std::string BPM;
    float difficulty                      = 0;
    uint32_t previewBegin                 = 0;
    uint32_t previewEnd                   = 0;
    uint16_t keyCount                     = 0;
    float baseBPM                         = 0.0f;
    std::pair<uint8_t, uint8_t> baseTempo = {0, 0};
};


