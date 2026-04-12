#pragma once

#include <map>
#include <cstddef>
#include <string>

// Lightweight JSON helper for flat string key-value objects.
class JsonUtils {
public:
    JsonUtils() = default;

    explicit JsonUtils(std::map<std::string, std::string> values);

    // Sets key-value pair.
    void set(const std::string &key, const std::string &value);

    // Gets value by key; returns defaultValue when key is missing.
    std::string get(const std::string &key, const std::string &defaultValue = "") const;

    // Returns whether the key exists.
    bool has(const std::string &key) const;

    // Returns all key-value pairs.
    const std::map<std::string, std::string> &values() const;

    // Parses JSON text (flat string object only).
    static bool parseFlatObject(const std::string &content, JsonUtils &out);

    // Loads JSON file (flat string object only).
    static bool loadFlatObjectFromFile(const std::string &filePath, JsonUtils &out);

    // Serializes to JSON text (flat string object only).
    static std::string stringifyFlatObject(const JsonUtils &json);

private:
    static void skipWhitespace(const std::string &s, std::size_t &pos);

    static bool parseJsonString(const std::string &s, std::size_t &pos, std::string &out);

    static std::string escapeJsonString(const std::string &s);

    std::map<std::string, std::string> entries;
};


