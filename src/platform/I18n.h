#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * Lightweight singleton i18n utility.
 *
 * Usage:
 *   I18n::instance().load("config/i18n/zh_CN.json");
 *   std::string label = I18n::instance().get("menu.start_game");
 *
 * Supported JSON format: flat object with string keys and string values.
 * UTF-8 source files are supported, including basic escapes (\", \\, \/, \n, \r, \t).
 *
 * If a key is missing, get() returns the key itself as a safe fallback.
 */
class I18n {
public:
    static I18n &instance();

    /**
     * Loads translation data from a UTF-8 JSON file.
     * Previously loaded translations are replaced.
     * @param filePath Path to i18n JSON file.
     * @return true on success; false if file is unreadable or JSON parsing fails.
     */
    bool load(const std::string &filePath);

    // Loads locale by environment-aware search order:
    //   1) development path: src/resources/i18n/
    //   2) production path: AppDirs::localeDir()
    // Falls back to en_US within each path group.
    bool ensureLocaleLoaded(const std::string &preferredLocale);

    /**
     * Looks up translation by key.
     * Returns key itself when not found.
     */
    std::string get(const std::string &key) const;

    /** Convenience alias for get(). */
    std::string operator()(const std::string &key) const;

    /** Locale identifier inferred from loaded filename, e.g. "zh_CN". */
    const std::string &locale() const;

    /**
     * Returns the list of locale IDs available on disk (stems of *.json files
     * in the dev i18n directory and/or AppDirs::localeDir()). Always sorted;
     * duplicates removed.  Returns at least {"en_US"} as a fallback.
     */
    static std::vector<std::string> listAvailableLocales();

private:
    I18n() = default;

    I18n(const I18n &) = delete;

    I18n &operator=(const I18n &) = delete;

    std::map<std::string, std::string> translations;
    std::string currentLocale;
};
