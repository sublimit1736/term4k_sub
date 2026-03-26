#ifndef TERM4K_USER_H
#define TERM4K_USER_H

#include <string>
#include <cstdint>

class User {
public:
    User() = default;

    User(uint32_t UID, std::string username, std::string password) : UID(UID), username(std::move(username)),
                                                                     password(std::move(password)) {}

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

const User Guest = User(0, "Guest", "");
const User Admin = User(1, "Admin", "");

#endif // TERM4K_USER_H