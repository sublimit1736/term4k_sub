#pragma once

#include "entities/User.h"

#include <optional>
#include <string>

class UserLoginInstance {
public:
    bool registerUser(const std::string &username, const std::string &password,
                      std::string* outErrorMessage = nullptr
        );

    bool login(const std::string &username, const std::string &password);

    void logout();

    bool isLoggedIn() const;

    std::optional<User> currentUser() const;
};