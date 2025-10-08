#include <stdlib.h>
#include "appstate.h"
#include "cute_tiled.h"

typedef struct SOT_Tilemap {
    cute_tiled_map_t *tilemap;
    SDL_Texture *tilesetTexture;
} SOT_Tilemap;

SOT_Tilemap *CreateTilemap(char *name, AppState *appState); 
void RenderTilemap(SOT_Tilemap *tilemap, AppState *appState);
void DestroyTilemap(SOT_Tilemap *tilemap);
