#ifndef APPSTATE_H
#define APPSTATE_H

#include <SDL3/SDL.h>

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint64 last_step;
    bool full_screen_enabled;
} AppState;

 #endif