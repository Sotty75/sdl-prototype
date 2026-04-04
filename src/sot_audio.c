#include "sot_audio.h"
#include "sot_common.h"

// ---- Internal helpers ----

static SOT_AudioClip *FindCachedClip(SOT_Audio *audio, const char *filename)
{
    for (int i = 0; i < audio->clipCacheCount; i++) {
        if (SDL_strcmp(audio->clipCache[i].filename, filename) == 0)
            return &audio->clipCache[i];
    }
    return NULL;
}

static SOT_AudioClip *LoadAndCacheClip(SOT_Audio *audio, const char *filename)
{
    SOT_AudioClip *existing = FindCachedClip(audio, filename);
    if (existing) return existing;

    if (audio->clipCacheCount >= SOT_AUDIO_MAX_CACHED) {
        SDL_Log("SOT_Audio: Clip cache full, cannot load '%s'", filename);
        return NULL;
    }

    // Build full path
    char fullPath[512];
    SDL_snprintf(fullPath, sizeof(fullPath), "%s\\%s", Paths.Base, filename);

    SOT_AudioClip *clip = &audio->clipCache[audio->clipCacheCount];
    SDL_strlcpy(clip->filename, filename, sizeof(clip->filename));

    if (!SDL_LoadWAV(fullPath, &clip->spec, &clip->data, &clip->dataLen)) {
        SDL_Log("SOT_Audio: Failed to load WAV '%s': %s", fullPath, SDL_GetError());
        return NULL;
    }

    audio->clipCacheCount++;
    return clip;
}

static int FindFreeChannel(SOT_Audio *audio)
{
    for (int i = 0; i < SOT_AUDIO_MAX_SFX_CHANNELS; i++) {
        if (!audio->sfxChannels[i].active)
            return i;
    }
    // Evict oldest (channel 0)
    if (audio->sfxChannels[0].stream) {
        SDL_DestroyAudioStream(audio->sfxChannels[0].stream);
        audio->sfxChannels[0].stream = NULL;
    }
    audio->sfxChannels[0].active = false;
    return 0;
}

// ---- Lifecycle ----

bool SOT_Audio_Init(SOT_Audio *audio)
{
    SDL_memset(audio, 0, sizeof(SOT_Audio));

    audio->masterVolume = 1.0f;
    audio->musicVolume = 0.7f;
    audio->sfxVolume = 1.0f;

    // Initialize audio subsystem if not already done
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            SDL_Log("SOT_Audio: Failed to init audio subsystem: %s", SDL_GetError());
            return false;
        }
    }

    // Open default audio device
    audio->deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (audio->deviceId == 0) {
        SDL_Log("SOT_Audio: Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    audio->initialized = true;
    SDL_Log("SOT_Audio: Initialized (device=%u)", audio->deviceId);
    return true;
}

void SOT_Audio_Shutdown(SOT_Audio *audio)
{
    if (!audio->initialized) return;

    // Stop music
    SOT_Audio_StopMusic(audio);

    // Stop all SFX
    for (int i = 0; i < SOT_AUDIO_MAX_SFX_CHANNELS; i++) {
        if (audio->sfxChannels[i].stream) {
            SDL_DestroyAudioStream(audio->sfxChannels[i].stream);
            audio->sfxChannels[i].stream = NULL;
        }
    }

    // Free cached clips
    for (int i = 0; i < audio->clipCacheCount; i++) {
        if (audio->clipCache[i].data)
            SDL_free(audio->clipCache[i].data);
    }

    if (audio->deviceId)
        SDL_CloseAudioDevice(audio->deviceId);

    audio->initialized = false;
    SDL_Log("SOT_Audio: Shutdown");
}

// ---- Music ----

bool SOT_Audio_PlayMusic(SOT_Audio *audio, const char *filename, bool loop)
{
    if (!audio->initialized) return false;

    SOT_Audio_StopMusic(audio);

    SOT_AudioClip *clip = LoadAndCacheClip(audio, filename);
    if (!clip) return false;

    SDL_AudioSpec dstSpec = { SDL_AUDIO_S16, 2, SOT_AUDIO_SAMPLE_RATE };
    audio->musicStream = SDL_CreateAudioStream(&clip->spec, &dstSpec);
    if (!audio->musicStream) {
        SDL_Log("SOT_Audio: Failed to create music stream: %s", SDL_GetError());
        return false;
    }

    SDL_BindAudioStream(audio->deviceId, audio->musicStream);
    SDL_PutAudioStreamData(audio->musicStream, clip->data, clip->dataLen);

    float effectiveVol = audio->masterVolume * audio->musicVolume;
    SDL_SetAudioStreamGain(audio->musicStream, effectiveVol);

    audio->musicClip = clip;
    audio->musicPlaying = true;
    audio->musicLooping = loop;

    SDL_Log("SOT_Audio: Playing music '%s' (loop=%d)", filename, loop);
    return true;
}

void SOT_Audio_StopMusic(SOT_Audio *audio)
{
    if (audio->musicStream) {
        SDL_DestroyAudioStream(audio->musicStream);
        audio->musicStream = NULL;
    }
    audio->musicPlaying = false;
    audio->musicClip = NULL;
}

void SOT_Audio_PauseMusic(SOT_Audio *audio, bool pause)
{
    if (!audio->musicStream) return;
    if (pause)
        SDL_PauseAudioStreamDevice(audio->musicStream);
    else
        SDL_ResumeAudioStreamDevice(audio->musicStream);
    audio->musicPlaying = !pause;
}

// ---- SFX ----

int SOT_Audio_PlaySFX(SOT_Audio *audio, const char *filename, float volume)
{
    if (!audio->initialized) return -1;

    SOT_AudioClip *clip = LoadAndCacheClip(audio, filename);
    if (!clip) return -1;

    int ch = FindFreeChannel(audio);

    SDL_AudioSpec dstSpec = { SDL_AUDIO_S16, 2, SOT_AUDIO_SAMPLE_RATE };
    audio->sfxChannels[ch].stream = SDL_CreateAudioStream(&clip->spec, &dstSpec);
    if (!audio->sfxChannels[ch].stream) {
        SDL_Log("SOT_Audio: Failed to create SFX stream: %s", SDL_GetError());
        return -1;
    }

    SDL_BindAudioStream(audio->deviceId, audio->sfxChannels[ch].stream);
    SDL_PutAudioStreamData(audio->sfxChannels[ch].stream, clip->data, clip->dataLen);

    float effectiveVol = audio->masterVolume * audio->sfxVolume * volume;
    SDL_SetAudioStreamGain(audio->sfxChannels[ch].stream, effectiveVol);

    audio->sfxChannels[ch].active = true;
    audio->sfxChannels[ch].volume = volume;
    audio->sfxChannels[ch].loop = false;

    return ch;
}

// ---- Volume ----

void SOT_Audio_SetMasterVolume(SOT_Audio *audio, float volume)
{
    audio->masterVolume = SDL_clamp(volume, 0.0f, 1.0f);
}

void SOT_Audio_SetMusicVolume(SOT_Audio *audio, float volume)
{
    audio->musicVolume = SDL_clamp(volume, 0.0f, 1.0f);
    if (audio->musicStream) {
        float effectiveVol = audio->masterVolume * audio->musicVolume;
        SDL_SetAudioStreamGain(audio->musicStream, effectiveVol);
    }
}

void SOT_Audio_SetSFXVolume(SOT_Audio *audio, float volume)
{
    audio->sfxVolume = SDL_clamp(volume, 0.0f, 1.0f);
}

// ---- Update (clean up finished channels, handle music loop) ----

void SOT_Audio_Update(SOT_Audio *audio)
{
    if (!audio->initialized) return;

    // Check music looping
    if (audio->musicPlaying && audio->musicLooping && audio->musicStream) {
        int queued = SDL_GetAudioStreamQueued(audio->musicStream);
        if (queued == 0 && audio->musicClip) {
            SDL_PutAudioStreamData(audio->musicStream, audio->musicClip->data, audio->musicClip->dataLen);
        }
    }

    // Clean up finished SFX channels
    for (int i = 0; i < SOT_AUDIO_MAX_SFX_CHANNELS; i++) {
        if (!audio->sfxChannels[i].active) continue;
        if (!audio->sfxChannels[i].stream) {
            audio->sfxChannels[i].active = false;
            continue;
        }

        int queued = SDL_GetAudioStreamQueued(audio->sfxChannels[i].stream);
        if (queued == 0) {
            SDL_DestroyAudioStream(audio->sfxChannels[i].stream);
            audio->sfxChannels[i].stream = NULL;
            audio->sfxChannels[i].active = false;
        }
    }
}
