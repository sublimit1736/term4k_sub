#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "entities/User.h"

// User session service (all-static).
// Provides registration, login, and in-memory session management.
class UserLoginService {
public:
    // Registers a normal user; UID is assigned sequentially from 1000.
    // If outErrorMessage is non-null, a readable failure reason is returned.
    static bool registerUser(const std::string &username, const std::string &password, uint32_t uid,
                             std::string* outErrorMessage = nullptr
        );

    // Logs in and stores current session in memory on success.
    static bool login(const std::string &username, const std::string &password);

    // Logs in as built-in admin account.
    static void loginAdmin();

    // Logs in as guest account.
    static void loginGuest();

    // Clears current login session.
    static void logout();

    // Returns whether current session is guest.
    static bool isGuest();

    // Returns whether current session is admin.
    static bool isAdmin();

    // Current UID as string; empty when guest or not logged in.
    static std::string currentUIDString();

    // Returns current user object; empty optional when not logged in.
    static std::optional<User> currentUser();
};


