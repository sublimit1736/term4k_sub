#pragma once

#include "chart/ChartCatalog.h"
#include "data/User.h"

#include <optional>
#include <string>
#include <vector>

class UserContext {
public:
    static bool syncFromUserLoginService();

    static void logout();

    static bool hasLoggedInUser();

    static std::optional<User> currentUser();

    static bool isAdminUser();

    static bool isGuestUser();

    static ChartRecordCollection loadCurrentUserVerifiedRecords(const std::string &chartsRoot);

    static std::vector<std::string> loadAllVerifiedRecords();

    static std::vector<std::string> loadAllRecords();
};
