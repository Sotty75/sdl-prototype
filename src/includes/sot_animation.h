#ifndef SOT_SPRITES_H
#define SOT_SPRITES_H

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "common.h"
#include "sot_texture.h"
#include "sot_gpu_pipeline.h"

/**
 * @brief Represents a single animation sequence (e.g., "Walk", "Jump").
 * 
 * This structure holds the name of the specific animation sequence and the 
 * list of texture coordinates (frames) that make up the animation.
 * 
 * @param name The name of the animation sequence (e.g., "Idle", "Run").
 * @param frames Dynamic array of vec4 (x, y, w, h) defining the source rectangle for each frame on the atlas.
 * @param framesCount The total number of frames in this sequence.
 */
typedef struct SOT_AnimationData {
	char *name;
	vec4 *frames;
	uint16_t count;
	uint16_t current;
} SOT_AnimationData;

/**
 * @brief Holds the raw data loaded from an animation definition file (JSON).
 * 
 * This structure contains metadata about the texture atlas used for the animations,
 * paths to related assets (image, collider), and a collection of frame sequences
 * defining individual animations.
 * 
 * @param atlasName Name of the texture atlas.
 * @param atlasPath File path to the texture atlas image.
 * @param collider Name or path of the associated collider definition.
 * @param step_ms Default duration of a frame in milliseconds.
 * @param framesInfo Fixed-size array containing details for up to 128 distinct animation sequences.
 */
typedef struct SOT_AnimationInfo {
	char* atlasName;
	char* atlasPath;
	char* collider;
    uint16_t step_ms;
	uint16_t count;
	SOT_AnimationData data[128];
	
} SOT_AnimationInfo;

SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename);


/**
 * @brief Represents a runtime instance of a specific animation sequence.
 * 
 * This structure binds a specific sequence of frames (e.g., "Walk") to the 
 * texture atlas and timing information required to play it. It also holds 
 * references to the GPU-specific data needed for rendering.
 * 
 * @param id Unique identifier for this animation instance.
 * @param atlasName Name of the texture atlas used.
 * @param step_ms Duration of a single frame in milliseconds.
 * @param frames The specific frame sequence data (name, count, and texture coordinates).
 * @param info Pointer to the GPU-specific sprite information corresponding to the frames.
 */
typedef struct SOT_Animation {
    int id;
    char *atlasName;
    uint16_t step_ms;
    SOT_AnimationData data;
    SOT_GPU_SpriteInfo *gpuInfo;  //-->collection of frames for the current Animation
} SOT_Animation;

// Prototipi delle funzioni che usano la struttura
SOT_Animation *CreateAnimation(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, int stepRateMillis, bool cycle, AppState* appState);
void DestroyAnimation(SOT_Animation *animation);


#endif // SPRITES_H