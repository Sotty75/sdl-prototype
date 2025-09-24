#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL3/SDL.h>

// Forward-declare the SOT_Texture struct
// This tells the compiler that "struct SOT_Texture" is a valid type
struct SOT_Texture;
struct Actor;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint64 last_step;
    bool full_screen_enabled;
    struct SOT_Texture *texturesPool;
} AppState;

 #endif