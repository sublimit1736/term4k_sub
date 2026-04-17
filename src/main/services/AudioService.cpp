#include "AudioService.h"
#include "I18nService.h"
#include "utils/ErrorNotifier.h"
#include <algorithm>
#include <cstdint>

// miniaudio callback (out-of-class): reads PCM frames from decoder into output buffer.
void AudioService::data_callback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount) {
    auto* pDecoder = static_cast<ma_decoder *>(pDevice->pUserData);
    if (pDecoder == nullptr){
        return;
    }
    ma_data_source_read_pcm_frames(pDecoder, pOutput, frameCount, nullptr);
}

AudioService::AudioService() {
    initialized = false;
    paused      = false;
    volume      = 1.0;
}

AudioService::~AudioService() {
    stop();
}

// Loads audio file and initializes playback device.
bool AudioService::loadSong(const char* filename) {
    if (initialized) stop();
    ma_result result = ma_decoder_init_file(filename, nullptr, &decoder);
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
    config.pUserData         = &decoder;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS){
        ErrorNotifier::notify(I18nService::instance().get("error.device_init_failed"));
        ma_decoder_uninit(&decoder);
        initialized = false;
        return false;
    }

    ma_device_set_master_volume(&device, volume);

    initialized = true;
    paused      = false;
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

// Stops playback and releases audio resources.
void AudioService::stop() {
    if (initialized){
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        initialized = false;
        paused      = false;
    }
}
