#include "Rating.h"

#include "utils/ErrorNotifier.h"
#include "utils/JsonUtils.h"

#include <sstream>
#include <utility>
#include <vector>

using Json = JsonUtils;

Rating::Rating() = default;

Rating::Rating(std::string chartID, std::string chartDisplayName, const uint32_t UID, std::string username,
               const uint32_t timeStamp, const uint32_t score, const float accuracy
    ) : UID(UID), timeStamp(timeStamp), username(std::move(username)), chartID(std::move(chartID)),
        chartDisplayName(std::move(chartDisplayName)), score(score), accuracy(accuracy) {}

uint32_t Rating::getTimeStamp() const { return timeStamp; }
uint32_t Rating::getUID() const { return UID; }
std::string Rating::getUsername() const { return username; }
std::string Rating::getChartID() const { return chartID; }
std::string Rating::getChartDisplayName() const { return chartDisplayName; }
uint32_t Rating::getScore() const { return score; }
float Rating::getAccuracy() const { return accuracy; }

void Rating::setTimeStamp(uint32_t value) { timeStamp = value; }
void Rating::setUID(uint32_t value) { UID = value; }
void Rating::setUsername(const std::string &value) { username = value; }
void Rating::setChartID(const std::string &value) { chartID = value; }
void Rating::setChartDisplayName(const std::string &value) { chartDisplayName = value; }
void Rating::setScore(uint32_t value) { score = value; }
void Rating::setAccuracy(float value) { accuracy = value; }

// Serialize rating record as a flat JSON string.
std::string Rating::serializeString() const {
    Json json;
    json.set("chartID", chartID);
    json.set("chartDisplayName", chartDisplayName);
    json.set("UID", std::to_string(UID));
    json.set("username", username);
    json.set("score", std::to_string(score));
    json.set("accuracy", std::to_string(accuracy));
    json.set("timeStamp", std::to_string(timeStamp));
    return Json::stringifyFlatObject(json);
}

// Deserialize from flat JSON first; if that fails, fall back to legacy space-separated format.
void Rating::deserializeFromString(const std::string &s) {
    Json json;
    if (Json::parseFlatObject(s, json)){
        chartID          = json.get("chartID", chartID);
        chartDisplayName = json.get("chartDisplayName", chartDisplayName);
        username         = json.get("username", username);

        try{ UID = static_cast<uint32_t>(std::stoul(json.get("UID", std::to_string(UID)))); }
        catch (...){}
        try{ score = static_cast<uint32_t>(std::stoul(json.get("score", std::to_string(score)))); }
        catch (...){}
        try{ accuracy = std::stof(json.get("accuracy", std::to_string(accuracy))); }
        catch (...){}
        try{ timeStamp = static_cast<uint32_t>(std::stoul(json.get("timeStamp", std::to_string(timeStamp)))); }
        catch (...){}
        return;
    }

    // Legacy format:
    // chartID chartDisplayName UID username score accuracy timeStamp
    std::istringstream iss(s);
    if (iss >> chartID >> chartDisplayName >> UID >> username >> score >> accuracy >> timeStamp){
        return;
    }

    // Alternate legacy format without UID:
    // chartID chartDisplayName username score accuracy timeStamp
    std::istringstream iss2(s);
    std::vector<std::string> fields;
    std::string token;
    while (iss2 >> token) fields.push_back(token);

    if (fields.size() >= 6){
        chartID          = fields[0];
        chartDisplayName = fields[1];
        username         = fields[2];
        try{ score = static_cast<uint32_t>(std::stoul(fields[3])); }
        catch (...){}
        try{ accuracy = std::stof(fields[4]); }
        catch (...){}
        try{ timeStamp = static_cast<uint32_t>(std::stoul(fields[5])); }
        catch (...){}
        UID = 0;
    }
}

Rating Rating::deserializeString(const std::string &s) {
    Rating r;
    r.deserializeFromString(s);
    return r;
}
