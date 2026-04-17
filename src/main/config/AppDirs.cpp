#include "AppDirs.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>

namespace fs = std::filesystem;

// -- Static member definitions -------------------------------------------------

std::string AppDirs::dataDirValue;
std::string AppDirs::chartsDirValue;
std::string AppDirs::configDirValue;
std::string AppDirs::localeDirValue;
bool AppDirs::initialized = false;

// -- Helper functions ----------------------------------------------------------

// Ensures the path ends with exactly one '/'.
static std::string ensureTrailingSlash(std::string p) {
    if (p.empty() || p.back() != '/') p += '/';
    return p;
}

// Returns an environment variable or fallback when missing/empty.
static std::string getenv_or(const char* var, const std::string &fallback) {
    const char* val = std::getenv(var);
    return (val != nullptr && val[0] != '\0') ? std::string(val) : fallback;
}

// Creates a directory if missing (including parents).
static void ensureDir(const std::string &path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    // Errors are intentionally ignored here and handled by later file operations.
}

// -- AppDirs::exeDir -----------------------------------------------------------

std::string AppDirs::exeDir() {
    std::array<char, 4096> buf{};
    const ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (len > 0){
        std::string path(buf.data(), static_cast<std::size_t>(len));
        const auto pos = path.rfind('/');
        if (pos != std::string::npos) return ensureTrailingSlash(path.substr(0, pos));
    }
    return "./";
}

// -- AppDirs::init flow --------------------------------------------------------

void AppDirs::init() {
    if (initialized) return;

    bool systemMode = false;

    // Detect system installation by checking /etc/term4k.
    const fs::path sysConfig = "/etc/term4k";
    if (fs::exists(sysConfig) && fs::is_directory(sysConfig)){
        systemMode     = true;
        dataDirValue   = "/var/lib/term4k/";
        chartsDirValue = "/usr/share/term4k/charts/";
        configDirValue = "/etc/term4k/";
    }
    else{
        // User mode: derive paths from XDG base directories.
        const std::string home = getenv_or("HOME", ".");

        const std::string xdgData = getenv_or("XDG_DATA_HOME",
                                              home + "/.local/share");
        const std::string xdgConfig = getenv_or("XDG_CONFIG_HOME",
                                                home + "/.config");

        dataDirValue   = ensureTrailingSlash(xdgData + "/term4k");
        chartsDirValue = ensureTrailingSlash(xdgData + "/term4k/charts");
        configDirValue = ensureTrailingSlash(xdgConfig + "/term4k");

        // Create user directories on first run to avoid extra write-time checks.
        ensureDir(dataDirValue);
        ensureDir(chartsDirValue);
        ensureDir(configDirValue);
    }

    // Use FHS path in system installs and local bundle path in user mode.
    localeDirValue = systemMode
                         ? "/usr/share/term4k/i18n/"
                         : ensureTrailingSlash(exeDir() + "i18n");
    if (!systemMode) ensureDir(localeDirValue);

    initialized = true;
}

// -- Accessors ----------------------------------------------------------------

const std::string &AppDirs::dataDir() { return dataDirValue; }
const std::string &AppDirs::chartsDir() { return chartsDirValue; }
const std::string &AppDirs::configDir() { return configDirValue; }
const std::string &AppDirs::localeDir() { return localeDirValue; }