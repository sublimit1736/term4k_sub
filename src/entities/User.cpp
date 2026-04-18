#include "User.h"

#include <utility>

User::User() = default;

User::User(const uint32_t UID, std::string username, std::string password) : UID(UID), username(std::move(username)),
                                                                             password(std::move(password)) {}

uint32_t User::getUID() const {
    return UID;
}

std::string User::getUsername() const {
    return username;
}

std::string User::getPassword() const {
    return password;
}

void User::setUID(uint32_t value) {
    UID = value;
}

void User::setUsername(const std::string &value) {
    username = value;
}

void User::setPassword(const std::string &value) {
    password = value;
}
