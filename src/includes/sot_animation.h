#ifndef SOT_ANIMATION_H_
#define SOT_ANIMATION_H_

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "sot_common.h"
#include "sot_texture.h"
#include "sot_gpu_pipeline.h"

/**
 * @brief Represents a single animation sequence definition (e.g., "Walk", "Jump").
 *
 * This structure holds the name of the specific animation sequence and the
 * list of texture coordinates (frames) that make up the animation.
 * Runtime playback state (current frame) is stored separately in SOT_Animation.
 *
 * @param name The name of the animation sequence (e.g., "Idle", "Run").
 * @param frames Dynamic array of vec4 (x, y, w, h) defining the source rectangle for each frame on the atlas.
 * @param count The total number of frames in this sequence.
 */
typedef struct SOT_AnimationSequence {
	char *name;
	vec4 *frames;
	uint16_t count;
} SOT_AnimationSequence;

/**
 * @brief Holds the raw data loaded from an animation definition file (JSON).
 *
 * This structure contains metadata about the texture atlas used for the animations,
 * paths to related assets (image, collider), and a collection of frame sequences
 * defining individual animations. Owned by SOT_Actor.
 *
 * @param atlasName Name of the texture atlas.
 * @param atlasPath File path to the texture atlas image.
 * @param collider Name or path of the associated collider definition.
 * @param step_ms Default duration of a frame in milliseconds.
 * @param count The number of animation sequences loaded.
 * @param sequences Heap-allocated dynamic array of SOT_AnimationSequence objects.
 */
typedef struct SOT_AnimationInfo {
	char* atlasName;
	char* atlasPath;
	char* collider;
	uint16_t step_ms;
	uint16_t count;
	SOT_AnimationSequence *sequences;
} SOT_AnimationInfo;



/**
 * @brief Represents a runtime instance of a specific animation sequence.
 *
 * This structure holds playback state and references to animation definitions.
 * The definition data (atlasName, step_ms, sequence frames) is accessed via the
 * non-owning back-pointer to SOT_AnimationInfo.
 *
 * @param id Unique identifier for this animation instance.
 * @param info Non-owning back-pointer to the animation definition (owned by actor).
 * @param sequenceIndex Index into info->sequences[] indicating which animation is active.
 * @param currentFrame Index of the currently playing frame in the sequence.
 * @param elapsedMs Accumulated time since last frame advance (for timing).
 * @param isPlaying Whether this animation is currently playing.
 * @param atlasSize Dimensions of the texture atlas (for UV normalization).
 * @param atlasIndex GPU texture index for this atlas.
 */
typedef struct SOT_Animation {
	int id;
	SOT_AnimationInfo *info;
	int sequenceIndex;
	uint16_t currentFrame;
	uint32_t elapsedMs;
	bool isPlaying;
	ivec2 atlasSize;
	uint32_t atlasIndex;
} SOT_Animation;

// Forward declare SOT_Actor to avoid circular dependency
struct SOT_Actor;

// Function declarations
SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename);
SDL_AppResult SOT_ActorLoadAnimationFile(struct SOT_Actor *actor, char *animationFile);


#endif // SOT_ANIMATION_H_