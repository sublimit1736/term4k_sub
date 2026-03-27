#include "catch_amalgamated.hpp"

#include "utils/ErrorNotifier.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
class SinkGuard {
public:
    explicit SinkGuard(std::vector<std::string> &messages) {
        ErrorNotifier::setSink([&messages](const std::string &msg) {
            messages.push_back(msg);
        });
    }

    ~SinkGuard() {
        ErrorNotifier::setSink(nullptr);
    }
};
} // namespace

TEST_CASE("ErrorNotifier forwards plain and contextual messages", "[utils][ErrorNotifier]") {
    std::vector<std::string> messages;
    SinkGuard guard(messages);

    ErrorNotifier::notify("simple");
    ErrorNotifier::notify("ctx", "detail");

    REQUIRE(messages.size() == 2);
    REQUIRE(messages[0] == "simple");
    REQUIRE(messages[1] == "ctx: detail");
}

TEST_CASE("ErrorNotifier reports exceptions and unknown failures", "[utils][ErrorNotifier]") {
    std::vector<std::string> messages;
    SinkGuard guard(messages);

    const std::runtime_error ex("boom");
    ErrorNotifier::notifyException("work", ex);
    ErrorNotifier::notifyUnknown("mystery");

    REQUIRE(messages.size() == 2);
    REQUIRE(messages[0] == "work: boom");
    REQUIRE(messages[1] == "mystery: unknown exception");
}

