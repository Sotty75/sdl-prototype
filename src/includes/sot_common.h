#ifndef SOT_COMMON_H
#define SOT_COMMON_H

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cglm.h>
#include <cJSON.h>

#define SCREEN_WIDTH 1980.0f
#define SCREEN_HEIGHT 1080.0f

void InitializeAssetsLoader();

typedef struct {
	vec3 position;
	vec3 color;
	vec2 texCoords;
} vertex;

typedef struct SOT_FramesInfo {
	char *name;
	vec4 *frames;
	uint16_t framesCount;
} SOT_FramesInfo;

typedef struct SOT_SpritesheetInfo {
	char* atlasName;
	char* atlasPath;
	SOT_FramesInfo framesInfo[128];
} SOT_AnimationInfo;

SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename);

SDL_Surface* LoadImage(const char* imageFilename, int desiredChannels);

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	Uint32 samplerCount, 
	Uint32 uniformBufferCount,
	Uint32 storageBufferCount,
	Uint32 storageTextureCount
);

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
);
#endif