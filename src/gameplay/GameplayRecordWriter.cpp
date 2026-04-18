#include "GameplayRecordWriter.h"

#include "account/RecordStore.h"
#include "account/UserContext.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {
uint32_t nowUnixSec() {
    const auto now = std::chrono::system_clock::now();
    const auto sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    if (sec <= 0) return 0;
    if (sec > static_cast<long long>(std::numeric_limits<uint32_t>::max())) {
        return std::numeric_limits<uint32_t>::max();
    }
    return static_cast<uint32_t>(sec);
}

uint32_t clampScoreToUint32(const uint64_t score) {
    if (score > std::numeric_limits<uint32_t>::max()) return std::numeric_limits<uint32_t>::max();
    return static_cast<uint32_t>(score);
}

std::string accuracyToRecordString(const double accuracy) {
    const double clamped = std::clamp(accuracy, 0.0, 100.0);
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << clamped;
    return os.str();
}
} // namespace

bool GameplayRecordWriter::saveFinalResult(const GameplayFinalResult &result,
                                            const std::string &chartID,
                                            const std::string &songName,
                                            const uint32_t timestampSec) {
    if (chartID.empty()) return false;
    if (!UserContext::hasLoggedInUser()) return false;
    if (UserContext::isAdminUser() || UserContext::isGuestUser()) return false;

    const auto user = UserContext::currentUser();
    if (!user.has_value()) return false;

    const std::string uid = std::to_string(user->getUID());
    const std::string username = user->getUsername();
    const std::string song = songName.empty() ? chartID : songName;
    const uint32_t score = clampScoreToUint32(result.getScore());
    const uint32_t ts = timestampSec == 0 ? nowUnixSec() : timestampSec;

    std::ostringstream record;
    record << uid << ' '
           << chartID << ' '
           << song << ' '
           << username << ' '
           << score << ' '
           << accuracyToRecordString(result.getAccuracy()) << ' '
           << ts << ' '
           << result.getMaxCombo() << ' '
           << result.getChartNoteCount() << ' '
           << result.getPerfectCount() << ' '
           << result.getEarlyCount() << ' '
           << result.getLateCount();

    return RecordStore::addRecord(record.str());
}
