#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL3/SDL.h>
#include "sot_gpu_pipeline.h"

// Forward-declare the SOT_Texture struct
// This tells the compiler that "struct SOT_Texture" is a valid type
struct sot_actor_t;
struct sot_texture_t;
struct sot_collider_node_t;

typedef struct {
    bool displayColliders;
} sot_debug_info_t;

typedef struct AppState
{
    
    SOT_GPU_State *gpu;
    Uint64 last_step;
    bool full_screen_enabled;
    sot_debug_info_t debugInfo;
    struct sot_texture_t *pTexturesPool;
    struct sot_collider_node_t *pDynamicColliders;
    struct sot_collider_node_t *pStaticColliders;
} AppState;



 #endif