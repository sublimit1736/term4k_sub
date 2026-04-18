#include "UserContext.h"

#include "chart/ChartCatalog.h"
#include "UserSession.h"
#include "account/RecordStore.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <sstream>

namespace {
    std::optional<User> currentUserSession;
    bool isAdminSession = false;
    bool isGuestSession = false;

    struct ParsedRecord {
        std::string uid;
        std::string chartId;
        uint32_t score     = 0;
        float accuracy     = 0.0f;
        uint32_t timestamp = 0;
        uint32_t maxCombo  = 0;
        bool valid         = false;
    };

    ParsedRecord parseRecord(const std::string &record) {
        std::istringstream iss(record);
        std::vector<std::string> fields;
        std::string token;
        while (iss >> token){
            fields.push_back(token);
        }

        ParsedRecord parsed;
        if (fields.size() < 6) return parsed;

        const bool uidFormat           = (fields.size() >= 7) && string_utils::isDigitsOnly(fields[0]);
        const std::size_t uidIdx       = uidFormat ? 0 : static_cast<std::size_t>(-1);
        const std::size_t chartIdx     = uidFormat ? 1 : 0;
        const std::size_t scoreIdx     = uidFormat ? 4 : 3;
        const std::size_t accuracyIdx  = uidFormat ? 5 : 4;
        const std::size_t timestampIdx = uidFormat ? 6 : 5;
        const std::size_t maxComboIdx  = uidFormat ? 7 : 6;

        try{
            if (uidFormat) parsed.uid = fields.at(uidIdx);
            parsed.chartId   = fields.at(chartIdx);
            parsed.score     = static_cast<uint32_t>(std::stoul(fields.at(scoreIdx)));
            parsed.accuracy  = std::stof(fields.at(accuracyIdx));
            parsed.timestamp = static_cast<uint32_t>(std::stoul(fields.at(timestampIdx)));
            if (fields.size() > maxComboIdx){
                parsed.maxCombo = static_cast<uint32_t>(std::stoul(fields.at(maxComboIdx)));
            }
            parsed.valid = true;
        }
        catch (...){
            parsed.valid = false;
        }

        return parsed;
    }

    ChartRecordCollection buildRecordCollection(const std::vector<std::string> &records,
                                                const std::string &chartsRoot,
                                                const std::string &uid
        ) {
        ChartRecordCollection out;
        if (records.empty()) return out;

        const auto catalog = ChartCatalog::loadCatalogForUID(chartsRoot, uid);

        struct Row {
            std::string key;
            uint32_t timestamp = 0;
        };
        std::vector<Row> rows;
        rows.reserve(records.size());

        std::size_t serial = 0;
        for (const auto &raw: records){
            const ParsedRecord parsed = parseRecord(raw);
            if (!parsed.valid) continue;

            const std::string key = std::to_string(parsed.timestamp) + "#" + std::to_string(serial++);

            ChartRecordEntry entry;
            entry.uid       = uid;
            entry.chartID   = parsed.chartId;
            entry.score     = parsed.score;
            entry.accuracy  = parsed.accuracy;
            entry.timestamp = parsed.timestamp;
            entry.maxCombo  = parsed.maxCombo;

            const auto found = catalog.find(parsed.chartId);
            if (found != catalog.end()){
                entry.chart = found->second.chart;
            }
            else{
                entry.chart.setID(parsed.chartId);
                entry.chart.setDisplayName(parsed.chartId);
            }

            out.records[key] = std::move(entry);
            rows.push_back({key, parsed.timestamp});
        }

        std::ranges::stable_sort(rows, [](const Row &a, const Row &b) {
            return a.timestamp > b.timestamp;
        });
        out.order.reserve(rows.size());
        for (const auto &row: rows){
            out.order.push_back(row.key);
        }
        return out;
    }
}

bool UserContext::syncFromUserLoginService() {
    const auto user = UserSession::currentUser();
    if (!user.has_value()){
        logout();
        return false;
    }
    currentUserSession = user;
    isAdminSession     = UserSession::isAdmin();
    isGuestSession     = UserSession::isGuest();
    return true;
}

void UserContext::logout() {
    currentUserSession.reset();
    isAdminSession = false;
    isGuestSession = false;
}

bool UserContext::hasLoggedInUser() {
    return currentUserSession.has_value();
}

std::optional<User> UserContext::currentUser() {
    return currentUserSession;
}

bool UserContext::isAdminUser() {
    return isAdminSession;
}

bool UserContext::isGuestUser() {
    return isGuestSession;
}

ChartRecordCollection UserContext::loadCurrentUserVerifiedRecords(const std::string &chartsRoot) {
    if (!currentUserSession.has_value() || isAdminSession || isGuestSession) return {};

    const std::string uid = std::to_string(currentUserSession->getUID());
    const auto records    = RecordStore::readVerifiedRecordByUID(uid);
    return buildRecordCollection(records, chartsRoot, uid);
}

std::vector<std::string> UserContext::loadAllVerifiedRecords() {
    return RecordStore::readVerifiedRecord();
}

std::vector<std::string> UserContext::loadAllRecords() {
    return RecordStore::readAllRecord();
}
