#include "UserStatScene.h"

#include "account/UserContext.h"
#include "account/UserSession.h"
#include "utils/RatingUtils.h"

#include <algorithm>

bool UserStatScene::registerUser(const std::string &username,
                                    const std::string &password,
                                    std::string *outErrorMessage) {
    return UserSession::registerUser(username, password, 0, outErrorMessage);
}

bool UserStatScene::login(const std::string &username, const std::string &password) {
    if (!UserSession::login(username, password)) return false;
    if (!UserContext::syncFromUserLoginService()) return false;

    if (UserContext::isGuestUser()) {
        UserSession::logout();
        UserContext::logout();
        return false;
    }

    return true;
}

void UserStatScene::logout() {
    UserSession::logout();
    UserContext::logout();
}

bool UserStatScene::isLoggedIn() const {
    return UserContext::hasLoggedInUser();
}

std::optional<User> UserStatScene::currentUser() const {
    return UserContext::currentUser();
}

bool UserStatScene::refresh(const std::string &chartsRoot) {
    userRecords      = UserContext::loadCurrentUserVerifiedRecords(chartsRoot);
    currentRating    = 0.0;
    currentPotential = 0.0;

    if (userRecords.records.empty()) return false;

    std::vector<double> evaluations;
    evaluations.reserve(userRecords.records.size());
    for (const auto &key: userRecords.order){
        const auto it = userRecords.records.find(key);
        if (it == userRecords.records.end()) continue;
        evaluations.push_back(rating_utils::singleChartEvaluation(it->second.chart.getDifficulty(), it->second.accuracy));
    }

    std::ranges::sort(evaluations, std::greater<>());
    // Statistics logic: take top-50 evaluations and compute sum/average.
    const std::size_t b50Count = std::min<std::size_t>(50, evaluations.size());
    for (std::size_t i = 0; i < b50Count; ++i){
        currentRating += evaluations[i];
    }
    if (b50Count > 0){
        currentPotential = currentRating / static_cast<double>(b50Count);
    }
    return true;
}

const ChartRecordCollection &UserStatScene::records() const {
    return userRecords;
}

double UserStatScene::rating() const {
    return currentRating;
}

double UserStatScene::potential() const {
    return currentPotential;
}
