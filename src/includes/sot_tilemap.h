#include <stdlib.h>
#include "appstate.h"
#include "cute_tiled.h"

typedef struct sot_tilemap_t {
    cute_tiled_map_t *tilemap;
    SDL_Texture *tilesetTexture;
} sot_tilemap_t;

sot_tilemap_t *CreateTilemap(char *name, AppState *appState); 
void RenderTilemap(sot_tilemap_t *tilemap, AppState *appState);
void DestroyTilemap(sot_tilemap_t *tilemap);
