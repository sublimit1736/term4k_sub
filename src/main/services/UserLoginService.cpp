#include "UserLoginService.h"

#include "config/RuntimeConfigs.h"
#include "dao/UserAccountsDAO.h"
#include "I18nService.h"

#include <algorithm>
#include <cctype>

namespace {
    constexpr uint32_t kMinNormalUserUID = 1000;

    uint32_t nextSequentialUID() {
        uint32_t maxUid      = kMinNormalUserUID - 1;
        const auto usernames = UserAccountsDAO::readAllUsernames();
        for (const auto &name: usernames){
            uint32_t uid = 0;
            if (!UserAccountsDAO::tryGetUIDByUsername(name, uid)) continue;
            if (uid >= kMinNormalUserUID){
                maxUid = std::max(maxUid, uid);
            }
        }
        return maxUid + 1;
    }

    bool isIllegalUsernameCharacter(const unsigned char ch) {
        return std::iscntrl(ch) != 0 || std::isspace(ch) != 0 || ch == '/' || ch == '\\' || ch == ':' || ch == '*' ||
               ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '|';
    }

    bool validateUsername(const std::string &username, std::string* outErrorMessage) {
        if (username.empty()){
            if (outErrorMessage != nullptr) *outErrorMessage = I18nService::instance().get("error.username_empty");
            return false;
        }

        if (!std::ranges::all_of(username, [](char ch) {
            return !isIllegalUsernameCharacter(static_cast<unsigned char>(ch));
        })){
            if (outErrorMessage != nullptr){
                *outErrorMessage = I18nService::instance().get("error.username_illegal_chars");
            }
            return false;
        }

        return true;
    }
}

namespace {
    std::optional<User> currentUserSession;
    bool isGuestSession = false;
    bool isAdminSession = false;
}

bool UserLoginService::registerUser(const std::string &username,
                                    const std::string &password,
                                    uint32_t uid,
                                    std::string* outErrorMessage
    ) {
    (void) uid;

    if (!validateUsername(username, outErrorMessage)) return false;

    if (username == Admin.getUsername() || username == Guest.getUsername()){
        if (outErrorMessage != nullptr) *outErrorMessage = I18nService::instance().get("error.username_reserved");
        return false;
    }

    if (!UserAccountsDAO::addUser(username, password, nextSequentialUID())){
        if (outErrorMessage != nullptr)
            *outErrorMessage = I18nService::instance().
                get("error.username_exists_or_write_failed");
        return false;
    }

    if (outErrorMessage != nullptr) outErrorMessage->clear();
    return true;
}

bool UserLoginService::login(const std::string &username, const std::string &password) {
    if (username == Admin.getUsername() || username == Guest.getUsername()) return false;
    if (!UserAccountsDAO::verifyUser(username, password)) return false;
    uint32_t uid = 0;
    if (!UserAccountsDAO::tryGetUIDByUsername(username, uid)) return false;

    // Load runtime settings immediately after successful login.
    currentUserSession = User(uid, username, "");
    isGuestSession     = false;
    isAdminSession     = false;
    RuntimeConfigs::loadForUser(username);
    return true;
}

void UserLoginService::loginAdmin() {
    currentUserSession = Admin;
    isGuestSession     = false;
    isAdminSession     = true;
    RuntimeConfigs::loadForUser(Admin.getUsername());
}

void UserLoginService::loginGuest() {
    currentUserSession = Guest;
    isGuestSession     = true;
    isAdminSession     = false;
    // Guest always uses defaults and never loads persisted settings.
    RuntimeConfigs::resetToDefaults();
}

void UserLoginService::logout() {
    if (currentUserSession.has_value() && !isGuestSession){
        RuntimeConfigs::saveForUser(currentUserSession->getUsername());
    }

    currentUserSession.reset();
    isGuestSession = false;
    isAdminSession = false;
    RuntimeConfigs::resetToDefaults();
}

bool UserLoginService::isGuest() {
    return isGuestSession;
}

bool UserLoginService::isAdmin() {
    return isAdminSession;
}

std::string UserLoginService::currentUIDString() {
    if (!currentUserSession.has_value() || isGuestSession || currentUserSession->getUID() == 0) return "";
    return std::to_string(currentUserSession->getUID());
}

std::optional<User> UserLoginService::currentUser() {
    return currentUserSession;
}