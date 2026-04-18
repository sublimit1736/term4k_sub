#pragma once
#include "miniaudio.h"
#include <atomic>
#include <cstdint>

// Audio playback service for loading and controlling songs.
class AudioPlayer {
public:
    AudioPlayer();

    ~AudioPlayer();

    // Loads an audio file.
    bool loadSong(const char* filename);

    // Starts playback.
    bool playSong();

    // Pauses playback.
    void pause();

    // Resumes playback.
    void resume();

    // Sets master volume in range [0.0, 1.0].
    void setVolume(float vol);

    // Seeks to the target position in milliseconds.
    bool seekToMs(uint32_t ms);

    // Stops playback and releases current audio resources.
    void stopSong();

    // Returns the current playback position in milliseconds.
    // Thread-safe: may be called from any thread.
    uint32_t positionMs() const;

    // Returns true once the audio stream has reached the end.
    // Thread-safe: may be called from any thread.
    bool isFinished() const;

private:
    // miniaudio data callback (called from device thread).
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // Stops playback and releases audio resources.
    void stop();

    ma_decoder decoder{};
    ma_device device{};
    ma_device_config config{};
    bool initialized = false;
    bool paused      = false;
    float volume     = 1.0f;
    std::atomic<bool> finished_{false};
};
