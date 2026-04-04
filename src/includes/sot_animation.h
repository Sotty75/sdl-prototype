#ifndef SOT_ANIMATION_H_
#define SOT_ANIMATION_H_

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "sot_common.h"
#include "sot_texture.h"
#include "sot_gpu_pipeline.h"

// ---- Play modes ----
typedef enum SOT_PlayMode {
    SOT_PLAY_LOOP = 0,         // Loop continuously (default)
    SOT_PLAY_ONCE,             // Play once, stop on last frame
    SOT_PLAY_PING_PONG,        // Forward then reverse, repeating
    SOT_PLAY_ONCE_DESTROY,     // Play once, then signal actor destruction
} SOT_PlayMode;

// ---- Frame event (attached to a specific frame) ----
#define SOT_MAX_FRAME_EVENTS 4

typedef struct SOT_FrameEvent {
    char name[32];             // Event name (e.g., "footstep", "spawn_bullet")
} SOT_FrameEvent;

// ---- Single frame definition ----
typedef struct SOT_FrameDef {
    vec4 rect;                 // x, y, w, h on atlas
    uint16_t duration_ms;      // Per-frame duration override (0 = use sequence default)
    SOT_FrameEvent events[SOT_MAX_FRAME_EVENTS];
    int eventCount;
} SOT_FrameDef;

/**
 * Animation sequence definition (e.g., "walk_R", "idle_L").
 */
typedef struct SOT_AnimationSequence {
    char *name;
    vec4 *frames;              // Legacy: raw frame rects (kept for backward compat)
    SOT_FrameDef *frameDefs;   // Enhanced: per-frame data with events and timing
    uint16_t count;
    SOT_PlayMode playMode;
} SOT_AnimationSequence;

/**
 * Raw data loaded from an animation definition file (JSON).
 */
typedef struct SOT_AnimationInfo {
    char* atlasName;
    char* atlasPath;
    char* collider;
    uint16_t step_ms;          // Default frame duration in ms
    uint16_t count;
    SOT_AnimationSequence *sequences;
} SOT_AnimationInfo;

/**
 * Runtime playback state for a specific animation sequence instance.
 */
typedef struct SOT_Animation {
    int id;
    SOT_AnimationInfo *info;
    int sequenceIndex;
    uint16_t currentFrame;
    uint32_t elapsedMs;
    bool isPlaying;
    bool finished;             // True when ONCE/ONCE_DESTROY reaches last frame
    bool pingPongReverse;      // True when ping-pong is going backward
    ivec2 atlasSize;
    uint32_t atlasIndex;
} SOT_Animation;

// Forward declare SOT_Actor to avoid circular dependency
struct SOT_Actor;

// ---- Loading / Saving ----
SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename);
bool SOT_SaveAnimations(const SOT_AnimationInfo *info, const char *filename);
void SOT_FreeAnimationInfo(SOT_AnimationInfo *info);
SDL_AppResult SOT_ActorLoadAnimationFile(struct SOT_Actor *actor, char *animationFile);

// ---- Named lookup and playback control ----
// Play an animation by name on an actor. Returns true if found.
bool SOT_Animation_Play(struct SOT_Actor *actor, const char *animName);

// Stop the current animation (freeze on current frame)
void SOT_Animation_Stop(struct SOT_Actor *actor);

// Check if current animation is still playing
bool SOT_Animation_IsPlaying(const struct SOT_Actor *actor);

// Set playback speed multiplier (1.0 = normal)
void SOT_Animation_SetSpeed(struct SOT_Actor *actor, float multiplier);

// Advance animation by deltaMs. Returns fired event names (if any) via callback.
typedef void (*SOT_FrameEventCallback)(struct SOT_Actor *actor, const char *eventName);
void SOT_Animation_Update(struct SOT_Actor *actor, uint32_t deltaMs, SOT_FrameEventCallback callback);

#endif // SOT_ANIMATION_H_
