#include "AudioService.h"
#include "I18nService.h"
#include "utils/ErrorNotifier.h"
#include <algorithm>
#include <cstdint>

// miniaudio callback (out-of-class): reads PCM frames from decoder into output buffer.
void AudioService::data_callback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount) {
    auto* svc = static_cast<AudioService*>(pDevice->pUserData);
    if (svc == nullptr) return;

    ma_uint64 framesRead = 0;
    const ma_result result = ma_data_source_read_pcm_frames(&svc->decoder, pOutput, frameCount, &framesRead);

    if (framesRead < frameCount) {
        const ma_uint32 frameSize = ma_get_bytes_per_frame(svc->decoder.outputFormat,
                                                            svc->decoder.outputChannels);
        auto* dst = static_cast<ma_uint8*>(pOutput) + framesRead * frameSize;
        ma_silence_pcm_frames(dst, frameCount - framesRead, svc->decoder.outputFormat,
                               svc->decoder.outputChannels);
    }

    if (result == MA_AT_END || framesRead == 0) {
        svc->finished_.store(true, std::memory_order_relaxed);
    }
}

AudioService::AudioService() = default;

AudioService::~AudioService() {
    stop();
}

// Loads audio file and initializes playback device.
bool AudioService::loadSong(const char* filename) {
    if (initialized) stop();
    const auto result = ma_decoder_init_file(filename, nullptr, &decoder);
    if (result != MA_SUCCESS){
        ErrorNotifier::notify(I18nService::instance().get("error.decoder_init_failed"));
        initialized = false;
        return false;
    }
    ma_data_source_set_looping(&decoder, MA_FALSE);

    config                   = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = decoder.outputFormat;
    config.playback.channels = decoder.outputChannels;
    config.sampleRate        = decoder.outputSampleRate;
    config.dataCallback      = data_callback;
    config.pUserData         = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS){
        ErrorNotifier::notify(I18nService::instance().get("error.device_init_failed"));
        ma_decoder_uninit(&decoder);
        initialized = false;
        return false;
    }

    ma_device_set_master_volume(&device, volume);

    initialized = true;
    paused      = false;
    finished_.store(false, std::memory_order_relaxed);
    return true;
}

// Starts audio playback.
bool AudioService::playSong() {
    if (!initialized){
        ErrorNotifier::notify(I18nService::instance().get("error.device_not_initialized"));
        return false;
    }

    if (ma_device_start(&device) != MA_SUCCESS){
        ErrorNotifier::notify(I18nService::instance().get("error.device_start_failed"));
        return false;
    }

    paused = false;
    return true;
}

// Pauses playback.
void AudioService::pause() {
    if (initialized && !paused){
        ma_device_stop(&device);
        paused = true;
    }
}

// Resumes playback.
void AudioService::resume() {
    if (initialized && paused){
        ma_device_start(&device);
        paused = false;
    }
}

// Sets volume with automatic clamping to [0.0, 1.0].
void AudioService::setVolume(float vol) {
    volume = std::clamp(vol, 0.0f, 1.0f);
    if (initialized){
        ma_device_set_master_volume(&device, volume);
    }
}

bool AudioService::seekToMs(uint32_t ms) {
    if (!initialized){
        ErrorNotifier::notify("SongPlayer::seekToMs", I18nService::instance().get("error.device_not_initialized"));
        return false;
    }
    const ma_uint64 sampleRate  = decoder.outputSampleRate;
    const ma_uint64 targetFrame = (static_cast<ma_uint64>(ms) * sampleRate) / 1000ull;
    if (ma_data_source_seek_to_pcm_frame(&decoder, targetFrame) != MA_SUCCESS){
        ErrorNotifier::notify("SongPlayer::seekToMs", I18nService::instance().get("error.seek_failed"));
        return false;
    }
    return true;
}

void AudioService::stopSong() {
    stop();
}

void AudioService::stop() {
    if (initialized){
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        initialized = false;
        paused      = false;
        finished_.store(false, std::memory_order_relaxed);
    }
}

uint32_t AudioService::positionMs() const {
    if (!initialized) return 0;
    ma_uint64 cursor = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    if (ma_data_source_get_cursor_in_pcm_frames(const_cast<ma_decoder*>(&decoder), &cursor) != MA_SUCCESS) return 0;
    const ma_uint32 sampleRate = decoder.outputSampleRate;
    if (sampleRate == 0) return 0;
    return static_cast<uint32_t>((cursor * 1000ull) / sampleRate);
}

bool AudioService::isFinished() const {
    return finished_.load(std::memory_order_relaxed);
}
