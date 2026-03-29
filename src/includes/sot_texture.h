#ifndef SOT_TEXTURE_H_
#define SOT_TEXTURE_H_

#include <stdlib.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include "appstate.h"

typedef struct sot_texture_t {
    char *name;
    SDL_Texture *texture;
    struct sot_texture_t *next;
} sot_texture_t;

/* checks if the requested texture exists in the 
 texture pool.
 - If present, returns the texture.
 - If not present, creates the texture and returns a pointer
*/
SDL_AppResult GetSurfaceFromImage(SDL_Surface **surface, char *assetName);
void DestroyTexturePool(AppState* appState);

#endif 
