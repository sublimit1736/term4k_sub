#include "Chart.h"
#include "utils/JsonUtils.h"
#include "utils/ErrorNotifier.h"
#include <sstream>
#include <algorithm>
#include <cctype>

Chart::Chart() = default;

Chart::Chart(std::string i, std::string d, std::string a, std::string c, std::string b, float df,
             uint32_t pb, uint32_t pe, uint16_t kc, float bb,
             std::pair<uint8_t, uint8_t> bt
    ) : id(std::move(i)), displayName(std::move(d)), artist(std::move(a)), charter(std::move(c)), BPM(std::move(b)),
        difficulty(df), previewBegin(pb), previewEnd(pe), keyCount(kc), baseBPM(bb), baseTempo(bt) {}

std::string Chart::getID() const {
    return id;
}

std::string Chart::getDisplayName() const {
    return displayName;
}

std::string Chart::getArtist() const {
    return artist;
}

std::string Chart::getCharter() const {
    return charter;
}

std::string Chart::getBPM() const {
    return BPM;
}

float Chart::getDifficulty() const {
    return difficulty;
}

void Chart::setID(const std::string &value) {
    id = value;
}

void Chart::setDisplayName(const std::string &value) {
    displayName = value;
}

void Chart::setArtist(const std::string &value) {
    artist = value;
}

void Chart::setCharter(const std::string &value) {
    charter = value;
}

void Chart::setBPM(const std::string &value) {
    BPM = value;
}

void Chart::setDifficulty(float value) {
    difficulty = value;
}

uint32_t Chart::getPreviewBegin() const {
    return previewBegin;
}

uint32_t Chart::getPreviewEnd() const {
    return previewEnd;
}

uint16_t Chart::getKeyCount() const {
    return keyCount;
}

float Chart::getBaseBPM() const {
    return baseBPM;
}

std::pair<uint8_t, uint8_t> Chart::getBaseTempo() const {
    return baseTempo;
}

void Chart::setPreviewBegin(uint32_t value) {
    previewBegin = value;
}

void Chart::setPreviewEnd(uint32_t value) {
    previewEnd = value;
}

void Chart::setKeyCount(uint16_t value) {
    keyCount = value;
}

void Chart::setBaseBPM(float value) {
    baseBPM = value;
}

void Chart::setBaseTempo(std::pair<uint8_t, uint8_t> value) {
    baseTempo = value;
}

// Serialize chart metadata to string.
std::string Chart::serializeString() const {
    JsonUtils json;
    json.set("id", id);
    json.set("displayname", displayName);
    json.set("artist", artist);
    json.set("charter", charter);
    json.set("BPM", BPM);
    json.set("keyCount", std::to_string(keyCount));
    json.set("baseBPM", std::to_string(baseBPM));
    json.set("baseTempo", std::to_string(baseTempo.first) + "," +
                          std::to_string(baseTempo.second));
    json.set("difficulty", std::to_string(difficulty));
    json.set("previewBegin", std::to_string(previewBegin));
    json.set("previewEnd", std::to_string(previewEnd));
    return JsonUtils::stringifyFlatObject(json);
}

// Deserialize chart metadata from string (instance method).
void Chart::deserializeFromString(const std::string &s) {
    JsonUtils json;
    if (JsonUtils::parseFlatObject(s, json)){
        id          = json.get("id", id);
        displayName = json.get("displayname", json.get("displayName", displayName));
        artist      = json.get("artist", artist);
        charter     = json.get("charter", charter);
        BPM         = json.get("BPM", json.get("bpm", BPM));

        try{ difficulty = std::stof(json.get("difficulty", std::to_string(difficulty))); }
        catch (const std::exception &ex){
            ErrorNotifier::notifyException("Chart::deserializeFromString difficulty", ex);
            difficulty = 0;
        }
        try{ previewBegin = static_cast<uint32_t>(std::stoul(json.get("previewBegin", std::to_string(previewBegin)))); }
        catch (const std::exception &ex){
            ErrorNotifier::notifyException("Chart::deserializeFromString previewBegin", ex);
            previewBegin = 0;
        }
        try{ previewEnd = static_cast<uint32_t>(std::stoul(json.get("previewEnd", std::to_string(previewEnd)))); }
        catch (const std::exception &ex){
            ErrorNotifier::notifyException("Chart::deserializeFromString previewEnd", ex);
            previewEnd = 0;
        }
        try{ keyCount = static_cast<uint16_t>(std::stoul(json.get("keyCount", std::to_string(keyCount)))); }
        catch (const std::exception &ex){
            ErrorNotifier::notifyException("Chart::deserializeFromString keyCount", ex);
            keyCount = 0;
        }
        try{ baseBPM = std::stof(json.get("baseBPM", std::to_string(baseBPM))); }
        catch (const std::exception &ex){
            ErrorNotifier::notifyException("Chart::deserializeFromString baseBPM", ex);
            baseBPM = 0.0f;
        }

        const std::string tempo = json.get("baseTempo", "");
        if (!tempo.empty()){
            try{
                if (const auto comma = tempo.find(','); comma != std::string::npos){
                    baseTempo.first  = static_cast<uint8_t>(std::stoul(tempo.substr(0, comma)));
                    baseTempo.second = static_cast<uint8_t>(std::stoul(tempo.substr(comma + 1)));
                }
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString baseTempo", ex);
                baseTempo = {0, 0};
            }
        }
        return;
    }

    // Helper lambda to trim leading/trailing whitespace.
    auto trim = [](const std::string &str) {
        std::string s = str;
        // Trim left
        s.erase(s.begin(),
                std::ranges::find_if(s, [](unsigned char ch) { return std::isspace(ch) == 0; }));
        // Trim right
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return std::isspace(ch) == 0; }).base(),
                s.end());
        return s;
    };

    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line)){
        // Handle Windows line endings.
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip braces lines.
        if (line == "{" || line == "}") continue;

        // Parse "key: value" format.
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        const std::string key   = trim(line.substr(0, colon));
        const std::string value = trim(line.substr(colon + 1));

        if (key == "id"){
            id = value;
        }
        else if (key == "displayName" || key == "displayname"){
            displayName = value;
        }
        else if (key == "artist"){
            artist = value;
        }
        else if (key == "charter"){
            charter = value;
        }
        else if (key == "bpm" || key == "BPM"){
            BPM = value;
        }
        else if (key == "difficulty"){
            try{
                difficulty = std::stof(value);
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString legacy difficulty", ex);
                difficulty = 0;
            }
        }
        else if (key == "previewBegin"){
            try{
                previewBegin = static_cast<uint32_t>(std::stoul(value));
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString legacy previewBegin", ex);
                previewBegin = 0;
            }
        }
        else if (key == "previewEnd"){
            try{
                previewEnd = static_cast<uint32_t>(std::stoul(value));
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString legacy previewEnd", ex);
                previewEnd = 0;
            }
        }
        else if (key == "keyCount"){
            try{
                keyCount = static_cast<uint16_t>(std::stoul(value));
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString legacy keyCount", ex);
                keyCount = 0;
            }
        }
        else if (key == "baseBPM"){
            try{
                baseBPM = std::stof(value);
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString legacy baseBPM", ex);
                baseBPM = 0.0f;
            }
        }
        else if (key == "baseTempo"){
            try{
                if (const auto comma = value.find(','); comma != std::string::npos){
                    baseTempo.first  = static_cast<uint8_t>(std::stoul(value.substr(0, comma)));
                    baseTempo.second = static_cast<uint8_t>(std::stoul(value.substr(comma + 1)));
                }
            }
            catch (const std::exception &ex){
                ErrorNotifier::notifyException("Chart::deserializeFromString legacy baseTempo", ex);
                baseTempo = {0, 0};
            }
        }
    }
}

// Deserialize chart metadata from string (static factory method).
Chart Chart::deserializeString(const std::string &s) {
    Chart c;
    c.deserializeFromString(s);
    return c;
}
