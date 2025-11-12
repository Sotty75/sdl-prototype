#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL3/SDL.h>

// Forward-declare the SOT_Texture struct
// This tells the compiler that "struct SOT_Texture" is a valid type
struct sot_actor_t;
struct sot_texture_t;
struct sot_collider_node_t;

typedef struct {
    bool displayColliders;
} sot_debug_info_t;

typedef struct
{
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    SDL_GPUDevice *gpuDevice;
    SDL_GPUGraphicsPipeline *renderingPipeline;
    SDL_GPUBuffer *vertexBuffer;
    SDL_GPUBuffer *indexBuffer;
    SDL_GPUBuffer *uniformBuffer;
    SDL_GPUTexture *texture_1;
    SDL_GPUTexture *texture_2;
    SDL_GPUSampler *textureSampler_1;
    SDL_GPUSampler *textureSampler_2;
    Uint64 last_step;
    bool full_screen_enabled;
    sot_debug_info_t debugInfo;
    struct sot_texture_t *pTexturesPool;
    struct sot_collider_node_t *pDynamicColliders;
    struct sot_collider_node_t *pStaticColliders;
} AppState;



 #endif