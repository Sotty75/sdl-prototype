#ifndef SOT_AUDIO_H_
#define SOT_AUDIO_H_

#include <SDL3/SDL.h>
#include <stdbool.h>

// ---- Configuration ----
#define SOT_AUDIO_MAX_SFX_CHANNELS 16
#define SOT_AUDIO_SAMPLE_RATE      44100

// ---- Audio channel (one active sound) ----
typedef struct SOT_AudioChannel {
    SDL_AudioStream *stream;
    bool active;
    float volume;
    bool loop;
} SOT_AudioChannel;

// ---- Cached sound effect ----
#define SOT_AUDIO_MAX_CACHED 64

typedef struct SOT_AudioClip {
    char filename[128];
    Uint8 *data;
    Uint32 dataLen;
    SDL_AudioSpec spec;
} SOT_AudioClip;

// ---- Audio system state ----
typedef struct SOT_Audio {
    bool initialized;

    // Volume controls (0.0 to 1.0)
    float masterVolume;
    float musicVolume;
    float sfxVolume;

    // Music (single track)
    SDL_AudioStream *musicStream;
    SOT_AudioClip *musicClip;
    bool musicPlaying;
    bool musicLooping;

    // SFX channels
    SOT_AudioChannel sfxChannels[SOT_AUDIO_MAX_SFX_CHANNELS];

    // Clip cache
    SOT_AudioClip clipCache[SOT_AUDIO_MAX_CACHED];
    int clipCacheCount;

    // Audio device
    SDL_AudioDeviceID deviceId;
} SOT_Audio;

// ---- Lifecycle ----
bool SOT_Audio_Init(SOT_Audio *audio);
void SOT_Audio_Shutdown(SOT_Audio *audio);

// ---- Music ----
bool SOT_Audio_PlayMusic(SOT_Audio *audio, const char *filename, bool loop);
void SOT_Audio_StopMusic(SOT_Audio *audio);
void SOT_Audio_PauseMusic(SOT_Audio *audio, bool pause);

// ---- Sound effects ----
// Returns channel index, or -1 on failure
int SOT_Audio_PlaySFX(SOT_Audio *audio, const char *filename, float volume);

// ---- Volume ----
void SOT_Audio_SetMasterVolume(SOT_Audio *audio, float volume);
void SOT_Audio_SetMusicVolume(SOT_Audio *audio, float volume);
void SOT_Audio_SetSFXVolume(SOT_Audio *audio, float volume);

// ---- Update (call each frame to clean up finished channels) ----
void SOT_Audio_Update(SOT_Audio *audio);

#endif
