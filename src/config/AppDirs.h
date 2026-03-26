#pragma once

#include <string>

/**
 * Resolves runtime filesystem paths for term4k.
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
 * The following path is mode-independent and always fixed:
 *   localeDir -> <exeDir>/i18n/
 *
 * Runtime settings are stored by RuntimeConfigs under:
 *   $XDG_CONFIG_HOME/term4k/<username>_settings.json (fallback: ~/.config/term4k/)
 *
 * Call AppDirs::init() once before accessing these paths.
 * In user mode, init() also creates missing directories.
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