#include "User.h"

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