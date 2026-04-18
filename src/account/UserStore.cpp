#include "UserStore.h"
#include "platform/I18n.h"
#include "utils/ErrorNotifier.h"
#include "utils/LiteDBUtils.h"

#include <algorithm>
#include <fstream>
#include <exception>
#include <limits>
#include <openssl/rand.h>
#include <sstream>

std::string UserStore::accountList = "users.db";

// Sets account database path for tests and runtime redirection.
void UserStore::setDataDir(const std::string &dir) {
    if (dir.empty() || dir == "."){
        accountList = "users.db";
        return;
    }
    std::string d = dir;
    if (d.back() != '/' && d.back() != '\\') d += '/';
    accountList = d + "users.db";
}

// Extracts one whitespace-separated field from a line.
std::string UserStore::getField(const std::string &line, size_t idx) {
    std::istringstream iss(line);
    std::string token;
    for (size_t i = 0; i <= idx; ++i){
        if (!(iss >> token)) return {};
    }
    return token;
}

namespace {
    bool parseUint32Strict(const std::string &text, uint32_t &value) {
        if (text.empty()) return false;
        size_t pos = 0;
        try{
            const unsigned long parsed = std::stoul(text, &pos, 10);
            if (pos != text.size() || parsed > std::numeric_limits<uint32_t>::max()) return false;
            value = static_cast<uint32_t>(parsed);
            return true;
        }
        catch (const std::exception &ex){
            ErrorNotifier::notifyException("UserStore::parseUint32Strict", ex);
            return false;
        }
        catch (...){
            ErrorNotifier::notifyUnknown("UserStore::parseUint32Strict");
            return false;
        }
    }
}

// Adds user: unique username, persisted as salt + slowHash.
bool UserStore::addUser(const std::string &username, const std::string &password, uint32_t uid) {
    if (username.empty() || password.empty()){
        ErrorNotifier::notify("UserStore::addUser", I18n::instance().get("error.auth_empty_credentials"));
        return false;
    }

    {
        std::ifstream in(accountList);
        std::string line;
        while (std::getline(in, line)){
            if (getField(line, 0) == username){
                ErrorNotifier::notify("UserStore::addUser",
                                      I18n::instance().get("error.auth_username_exists") + ": " + username);
                return false;
            }
            uint32_t existingUid = 0;
            if (parseUint32Strict(getField(line, 1), existingUid) && existingUid == uid){
                ErrorNotifier::notify("UserStore::addUser",
                                      I18n::instance().get("error.auth_uid_exists") + ": " +
                                      std::to_string(uid));
                return false;
            }
        }
    }

    // Generate 16-byte random salt.
    std::vector<uint8_t> salt(16);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1){
        ErrorNotifier::notify("UserStore::addUser",
                              I18n::instance().get("error.auth_salt_generation_failed"));
        return false;
    }

    // Compute salted slow hash to avoid plaintext or fast-hash storage.
    std::vector<uint8_t> toHash = salt;
    toHash.insert(toHash.end(), password.begin(), password.end());
    const auto hashed = LiteDBUtils::slowHash(toHash);

    // File format: username uid saltHex hashHex
    std::ofstream out(accountList, std::ios::app);
    if (!out){
        ErrorNotifier::notify("UserStore::addUser",
                              I18n::instance().get("error.auth_account_file_open_failed") + ": " + accountList);
        return false;
    }
    out << username << " " << uid << " " << LiteDBUtils::hexEncode(salt) << " " << LiteDBUtils::hexEncode(hashed) <<
        "\n";
    return true;
}

// Verifies password by recomputing slowHash and using constant-time compare.
bool UserStore::verifyUser(const std::string &username, const std::string &password) {
    if (username.empty() || password.empty()){
        ErrorNotifier::notify("UserStore::verifyUser",
                              I18n::instance().get("error.auth_empty_credentials"));
        return false;
    }

    auto constantTimeEqual = [](const std::vector<uint8_t> &a, const std::vector<uint8_t> &b) {
        if (a.size() != b.size()) return false;
        uint8_t diff = 0;
        for (size_t i = 0; i < a.size(); ++i){
            diff |= static_cast<uint8_t>(a[i] ^ b[i]);
        }
        return diff == 0;
    };

    bool matched = false;
    std::ifstream in(accountList);
    std::string line;
    while (std::getline(in, line)){
        if (getField(line, 0) != username) continue;
        std::string saltHex;
        std::string hashHex;
        // Compatible old format: username saltHex hashHex
        // New format: username uid saltHex hashHex
        const bool hasUID = (std::count(line.begin(), line.end(), ' ') >= 3);
        if (hasUID){
            saltHex = getField(line, 2);
            hashHex = getField(line, 3);
        }
        else{
            saltHex = getField(line, 1);
            hashHex = getField(line, 2);
        }
        auto salt       = LiteDBUtils::hexDecode(saltHex);
        auto storedHash = LiteDBUtils::hexDecode(hashHex);
        if (salt.empty() || storedHash.empty()) continue;
        std::vector<uint8_t> material = salt;
        material.insert(material.end(), password.begin(), password.end());
        const auto computedHash = LiteDBUtils::slowHash(material);
        if (constantTimeEqual(computedHash, storedHash)){
            matched = true;
        }
    }
    return matched;
}

// Reads all usernames from account file.
std::vector<std::string> UserStore::readAllUsernames() {
    std::vector<std::string> usernames;
    std::ifstream in(accountList);
    std::string line;
    while (std::getline(in, line)){
        const auto username = getField(line, 0);
        if (!username.empty()) usernames.push_back(username);
    }
    return usernames;
}

bool UserStore::tryGetUIDByUsername(const std::string &username, uint32_t &uid) {
    if (username.empty()){
        ErrorNotifier::notify("UserStore::tryGetUIDByUsername",
                              I18n::instance().get("error.auth_username_empty"));
        return false;
    }
    std::ifstream in(accountList);
    std::string line;
    while (std::getline(in, line)){
        if (getField(line, 0) != username) continue;
        uint32_t parsed = 0;
        if (!parseUint32Strict(getField(line, 1), parsed)){
            uid = 0; // Old format compatibility (no UID field).
            return true;
        }
        uid = parsed;
        return true;
    }
    return false;
}
