#pragma once

#include <string>
#include <cstdint>

class User {
public:
    User();

    User(uint32_t UID, std::string username, std::string password);

    uint32_t getUID() const;

    std::string getUsername() const;

    std::string getPassword() const;

    void setUID(uint32_t value);

    void setUsername(const std::string &value);

    void setPassword(const std::string &value);

private:
    uint32_t UID = 0;
    std::string username;
    std::string password;
};

inline const User Guest = User(0, "Guest", "");
inline const User Admin = User(1, "Admin", "");

