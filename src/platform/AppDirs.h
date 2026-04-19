#pragma once

#include <string>

/**
 * Resolves runtime filesystem paths for term4k.
 *
 * Development mode (detected when src/resources/i18n/ exists in or above CWD):
 *   dataDir   -> <projectRoot>/data/
 *   chartsDir -> <projectRoot>/data/charts/
 *   configDir -> <projectRoot>/data/config/
 *   localeDir -> <projectRoot>/src/resources/i18n/
 *
 * System mode (detected when /etc/term4k/ exists, e.g. package install):
 *   dataDir   -> /var/lib/term4k/
 *   chartsDir -> /usr/share/term4k/charts/
 *   configDir -> /etc/term4k/
 *
 * User mode (XDG fallback when system install is not detected):
 *   dataDir   -> $XDG_DATA_HOME/term4k/        (fallback: ~/.local/share/term4k/)
 *   chartsDir -> $XDG_DATA_HOME/term4k/charts/ (fallback: ~/.local/share/term4k/charts/)
 *   configDir -> $XDG_CONFIG_HOME/term4k/      (fallback: ~/.config/term4k/)
 *
 * Runtime settings are stored by RuntimeConfig under:
 *   $configDir/<username>_settings.json
 *
 * Call AppDirs::init() once before accessing these paths.
 * init() also creates missing directories in dev and user modes.
 */
class AppDirs {
public:
    /**
     * Detects runtime mode (system/user) and initializes all paths.
     * Safe to call repeatedly; subsequent calls are no-ops.
     */
    static void init();

    /** Writable runtime data directory (records.db, proof.db, key.bin). */
    static const std::string &dataDir();

    /** User chart directory (.t4k files). */
    static const std::string &chartsDir();

    /** Application configuration directory. */
    static const std::string &configDir();

    /** I18n JSON directory (always <exeDir>/i18n/). */
    static const std::string &localeDir();

    /** Executable directory (always ends with '/'; Linux /proc/self/exe based). */
    static std::string exeDir();

private:
    static std::string dataDirValue;
    static std::string chartsDirValue;
    static std::string configDirValue;
    static std::string localeDirValue;
    static bool initialized;
};
