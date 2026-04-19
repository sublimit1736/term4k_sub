#pragma once

#include <cstdint>
#include <string>
#include <vector>

// User account database DAO (all-static service class).
// File format: username uid saltHex passwordHashHex
class UserStore {
public:
    // Sets the directory for users.db (empty string or "." restores current directory).
    static void setDataDir(const std::string &dir);

    // Adds a user: username must be unique; password is stored as salted slow-hash.
    static bool addUser(const std::string &username, const std::string &password, uint32_t uid);

    // Verifies a user by recomputing salted hash and comparing in constant time.
    static bool verifyUser(const std::string &username, const std::string &password);

    // Reads all usernames in file order.
    static std::vector<std::string> readAllUsernames();

    // Reads the UID for a username.
    // New format returns UID directly; old format falls back to 0 for compatibility.
    static bool tryGetUIDByUsername(const std::string &username, uint32_t &uid);

    // Returns true when the account database file can be opened for reading.
    static bool isAccountDBAccessible();

private:
    // Account database file path (overridable via setDataDir).
    static std::string accountList;

    // Extracts the idx-th whitespace-separated field (0-based).
    static std::string getField(const std::string &line, size_t idx);
};
