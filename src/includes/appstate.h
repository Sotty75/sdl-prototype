#ifndef SOT_APPSTATE_H_
#define SOT_APPSTATE_H_

#include <SDL3/SDL.h>
#include "sot_gpu_pipeline.h"
#include "sot_input.h"
#include "sot_lua.h"
#include "sot_audio.h"
#include "sot_ui.h"
#include "sot_editor.h"

// Forward-declare the SOT_Texture struct
// This tells the compiler that "struct SOT_Texture" is a valid type
struct SOT_Actor;
struct sot_texture_t;
struct sot_collider_node_t;

typedef struct {
    bool displayColliders;
} sot_debug_info_t;

typedef struct AppState
{
    SOT_GPU_State *gpu;
    SOT_Input input;
    SOT_Lua lua;
    SOT_Audio audio;
    SOT_UI ui;
    SOT_Editor editor;
    Uint64 last_step;
    bool full_screen_enabled;
    sot_debug_info_t debugInfo;
    struct sot_texture_t *pTexturesPool;
    struct sot_collider_node_t *pDynamicColliders;
    struct sot_collider_node_t *pStaticColliders;
} AppState;



 #endif