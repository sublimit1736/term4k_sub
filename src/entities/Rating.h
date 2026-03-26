#ifndef TERM4K_RATING_H
#define TERM4K_RATING_H

#include <cstdint>
#include <string>

// Score record entity.
class Rating {
public:
    // Default constructor.
    Rating() = default;

    // Full-argument constructor.
    Rating(std::string chartID, std::string chartDisplayName, uint32_t UID, std::string username,
        uint32_t timeStamp, uint32_t score, float accuracy) :
        UID(UID), timeStamp(timeStamp), username(username), chartID(chartID),
        score(score), accuracy(accuracy) {}

    // --- Getters ---
    uint32_t getTimeStamp() const;

    uint32_t getUID() const;

    std::string getUsername() const;

    std::string getChartID() const;

    std::string getChartDisplayName() const;

    uint32_t getScore() const;

    float getAccuracy() const;

    // --- Setters ---
    void setTimeStamp(uint32_t value);

    void setUID(uint32_t value);

    void setUsername(const std::string &value);

    void setChartID(const std::string &value);

    void setChartDisplayName(const std::string &value);

    void setScore(uint32_t value);

    void setAccuracy(float value);

    // Serializes to string.
    std::string serializeString() const;

    // Deserializes from string (instance method).
    void deserializeFromString(const std::string &s);

    // Deserializes from string (static factory method).
    static Rating deserializeString(const std::string &s);

private:
    uint32_t UID = 0;
    uint32_t timeStamp = 0;
    std::string username;
    std::string chartID;
    std::string chartDisplayName;
    uint32_t score = 0;
    float accuracy = 0.0f;
};

#endif // TERM4K_RATING_H