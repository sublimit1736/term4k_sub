#include "catch_amalgamated.hpp"

#include "services/AudioService.h"
#include "utils/ErrorNotifier.h"

#include <string>
#include <vector>

namespace {
    class SinkGuard {
    public:
        explicit SinkGuard(std::vector<std::string> &messages) {
            ErrorNotifier::setSink([&messages](const ErrorNotifier::Level, const std::string &msg) {
                messages.push_back(msg);
            });
        }

        ~SinkGuard() {
            ErrorNotifier::setSink(nullptr);
        }
    };
} // namespace

TEST_CASE (

"SongPlayer reports errors when used before initialization"
,
"[services][SongPlayer]"
)
 {
    AudioService player;
    std::vector<std::string> messages;
    SinkGuard guard(messages);

    REQUIRE_FALSE(player.playSong());
    REQUIRE_FALSE(player.seekToMs(500));

    REQUIRE(messages.size() >= 2);
    REQUIRE(messages[0].find("error.device_not_initialized") != std::string::npos);
}

TEST_CASE (

"SongPlayer rejects missing audio files"
,
"[services][SongPlayer]"
)
 {
    AudioService player;
    std::vector<std::string> messages;
    SinkGuard guard(messages);

    REQUIRE_FALSE(player.loadSong("/definitely/not/found/audio.mp3"));
    REQUIRE_FALSE(messages.empty());
}

TEST_CASE (

"SongPlayer control methods are safe before load"
,
"[services][SongPlayer]"
)
 {
    AudioService player;

    REQUIRE_NOTHROW(player.pause());
    REQUIRE_NOTHROW(player.resume());
    REQUIRE_NOTHROW(player.setVolume(-2.0f));
    REQUIRE_NOTHROW(player.setVolume(2.0f));
    REQUIRE_NOTHROW(player.stopSong());
}