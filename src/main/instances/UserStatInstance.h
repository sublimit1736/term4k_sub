#pragma once

#include "entities/User.h"
#include "services/ChartCatalogService.h"

#include <optional>
#include <string>

class UserStatInstance {
public:
    bool registerUser(const std::string &username,
                      const std::string &password,
                      std::string *outErrorMessage = nullptr);

    bool login(const std::string &username, const std::string &password);

    void logout();

    bool isLoggedIn() const;

    std::optional<User> currentUser() const;

    bool refresh(const std::string &chartsRoot);

    const ChartRecordCollection &records() const;

    double rating() const;

    double potential() const;

private:
    ChartRecordCollection userRecords;
    double currentRating    = 0.0;
    double currentPotential = 0.0;
};
