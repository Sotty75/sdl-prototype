#ifndef COMMON_H
#define COMMON_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "cglm.h"

#define QUAD_VERTS 4
#define QUAD_INDEXES 6

void InitializeAssetsLoader();

typedef struct {
	vec3 position;
	vec3 color;
	vec2 texCoords;
} vertex;

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