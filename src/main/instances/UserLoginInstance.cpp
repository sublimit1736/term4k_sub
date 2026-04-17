#include "UserLoginInstance.h"

#include "services/AuthenticatedUserService.h"
#include "services/UserLoginService.h"

bool UserLoginInstance::registerUser(const std::string &username,
                                     const std::string &password,
                                     std::string* outErrorMessage
    ) {
    return UserLoginService::registerUser(username, password, 0, outErrorMessage);
}

bool UserLoginInstance::login(const std::string &username, const std::string &password) {
    if (!UserLoginService::login(username, password)) return false;
    if (!AuthenticatedUserService::syncFromUserLoginService()) return false;

    if (AuthenticatedUserService::isGuestUser()){
        UserLoginService::logout();
        AuthenticatedUserService::logout();
        return false;
    }

    return true;
}

void UserLoginInstance::logout() {
    UserLoginService::logout();
    AuthenticatedUserService::logout();
}

bool UserLoginInstance::isLoggedIn() const {
    return AuthenticatedUserService::hasLoggedInUser();
}

std::optional<User> UserLoginInstance::currentUser() const {
    return AuthenticatedUserService::currentUser();
}
