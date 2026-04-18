#pragma once

#include "ChartCatalogService.h"
#include "entities/User.h"

#include <optional>
#include <string>
#include <vector>

class AuthenticatedUserService {
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
