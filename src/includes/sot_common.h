#ifndef SOT_COMMON_H
#define SOT_COMMON_H

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cglm.h>
#include <cJSON.h>

// Game rendering uses the internal (virtual framebuffer) resolution.
// These match SOT_INTERNAL_WIDTH/HEIGHT in sot_display.h.
#define SCREEN_WIDTH  320.0f
#define SCREEN_HEIGHT 240.0f

typedef struct SOT_Paths{
	char *Base;
	char *Textures;
	char *Shaders;
	char *TiledMaps;
	char *Animations;
	char *Scenes;
} SOT_Paths;

typedef struct {
	vec3 position;
	vec3 color;
	vec2 texCoords;
} vertex;

// Global Variables
extern SOT_Paths Paths;

void InitializeAssetsLoader();

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