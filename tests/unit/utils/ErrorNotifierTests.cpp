#include "catch_amalgamated.hpp"

#include "utils/ErrorNotifier.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
    struct Captured {
        ErrorNotifier::Level level = ErrorNotifier::Level::Info;
        std::string text;
    };

    class SinkGuard {
    public:
        explicit SinkGuard(std::vector<Captured> &messages) {
            ErrorNotifier::setSink([&messages](const ErrorNotifier::Level level, const std::string &msg) {
                messages.push_back({level, msg});
            });
        }

        ~SinkGuard() {
            ErrorNotifier::setSink(nullptr);
        }
    };
} // namespace

TEST_CASE (

"ErrorNotifier forwards plain and contextual messages"
,
"[utils][ErrorNotifier]"
)
 {
    std::vector<Captured> messages;
    SinkGuard guard(messages);

    ErrorNotifier::notify("simple");
    ErrorNotifier::notify("ctx", "detail");

    REQUIRE(messages.size() == 2);
    REQUIRE(messages[0].level == ErrorNotifier::Level::Error);
    REQUIRE(messages[0].text == "simple");
    REQUIRE(messages[1].level == ErrorNotifier::Level::Error);
    REQUIRE(messages[1].text == "ctx: detail");
}

TEST_CASE (

"ErrorNotifier reports exceptions and unknown failures"
,
"[utils][ErrorNotifier]"
)
 {
    std::vector<Captured> messages;
    SinkGuard guard(messages);

    const std::runtime_error ex("boom");
    ErrorNotifier::notifyException("work", ex);
    ErrorNotifier::notifyUnknown("mystery");

    REQUIRE(messages.size() == 2);
    REQUIRE(messages[0].level == ErrorNotifier::Level::Error);
    REQUIRE(messages[0].text == "work: boom");
    REQUIRE(messages[1].level == ErrorNotifier::Level::Error);
    REQUIRE(messages[1].text == "mystery: unknown exception");
}