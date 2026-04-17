#include "JsonUtils.h"

#include <cctype>
#include <fstream>
#include <sstream>

JsonUtils::JsonUtils(std::map<std::string, std::string> values) : entries(std::move(values)) {}

void JsonUtils::set(const std::string &key, const std::string &value) {
    entries[key] = value;
}

std::string JsonUtils::get(const std::string &key, const std::string &defaultValue) const {
    auto it = entries.find(key);
    return (it != entries.end()) ? it->second : defaultValue;
}

bool JsonUtils::has(const std::string &key) const {
    return entries.find(key) != entries.end();
}

const std::map<std::string, std::string> &JsonUtils::values() const {
    return entries;
}

void JsonUtils::skipWhitespace(const std::string &s, std::size_t &pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])) != 0) ++pos;
}

bool JsonUtils::parseJsonString(const std::string &s, std::size_t &pos, std::string &out) {
    if (pos >= s.size() || s[pos] != '"') return false;
    ++pos;

    out.clear();
    while (pos < s.size() && s[pos] != '"'){
        if (s[pos] == '\\'){
            ++pos;
            if (pos >= s.size()) return false;
            switch (s[pos]){
                case '"': out += '"';
                    break;
                case '\\': out += '\\';
                    break;
                case '/': out += '/';
                    break;
                case 'n': out += '\n';
                    break;
                case 'r': out += '\r';
                    break;
                case 't': out += '\t';
                    break;
                default: out += s[pos];
                    break;
            }
        }
        else{
            out += s[pos];
        }
        ++pos;
    }

    if (pos >= s.size()) return false;
    ++pos;
    return true;
}

bool JsonUtils::parseFlatObject(const std::string &content, JsonUtils &out) {
    out = JsonUtils{};

    std::size_t pos = 0;
    while (pos < content.size() && content[pos] != '{') ++pos;
    if (pos >= content.size()) return false;
    ++pos;

    std::string key;
    std::string value;
    while (pos < content.size()){
        skipWhitespace(content, pos);
        if (pos >= content.size()) break;

        const char c = content[pos];
        if (c == '}') return true;
        if (c == ','){
            ++pos;
            continue;
        }
        if (c != '"') return false;

        if (!parseJsonString(content, pos, key)) return false;

        skipWhitespace(content, pos);
        if (pos >= content.size() || content[pos] != ':') return false;
        ++pos;

        skipWhitespace(content, pos);
        if (!parseJsonString(content, pos, value)) return false;

        out.set(key, value);
    }

    return false;
}

bool JsonUtils::loadFlatObjectFromFile(const std::string &filePath, JsonUtils &out) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    return parseFlatObject(ss.str(), out);
}

std::string JsonUtils::escapeJsonString(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char ch: s){
        switch (ch){
            case '"': out += "\\\"";
                break;
            case '\\': out += "\\\\";
                break;
            case '\n': out += "\\n";
                break;
            case '\r': out += "\\r";
                break;
            case '\t': out += "\\t";
                break;
            default: out += ch;
                break;
        }
    }
    return out;
}

std::string JsonUtils::stringifyFlatObject(const JsonUtils &json) {
    std::string out = "{";
    bool first      = true;
    for (const auto &kv: json.values()){
        if (!first) out += ",";
        first = false;
        out   += "\"" + escapeJsonString(kv.first) + "\":\"" + escapeJsonString(kv.second) + "\"";
    }
    out += "}";
    return out;
}
