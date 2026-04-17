#pragma once

#include "entities/User.h"

#include <optional>
#include <string>

class UserLoginInstance {
public:
    static bool registerUser(const std::string &username, const std::string &password,
                      std::string* outErrorMessage = nullptr
        );

    static bool login(const std::string &username, const std::string &password);

    static void logout();

    static bool isLoggedIn();

    static std::optional<User> currentUser();
};
