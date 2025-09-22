#ifndef sto_texture_manager_h
#define sto_texture_manager_h

#include <stdlib.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include "appstate.h"

typedef struct SOT_Texture {
    char *name;
    SDL_Texture *texture;
    struct SOT_Texture *next;
} SOT_Texture;

/* checks if the requested texture exists in the 
 texture pool.
 - If present, returns the texture.
 - If not present, creates the texture and returns a pointer
*/
SDL_Texture *GetTexture(char *name, AppState* appState);
SDL_AppResult GetSurfaceFromImage(SDL_Surface **surface, char *assetName);
SOT_Texture *CreateTexture(char *name, AppState* appState);
void DestroyTexturePool(AppState* appState);

#endif 
