#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL3/SDL.h>

// Forward-declare the SOT_Texture struct
// This tells the compiler that "struct SOT_Texture" is a valid type
struct sot_texture_t;
struct sot_actor_t;

typedef struct
{
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    Uint64 last_step;
    bool full_screen_enabled;
    struct sot_texture_t *pTexturesPool;
} AppState;

 #endif