#include <stdlib.h>
#include "appstate.h"
#include "cute_tiled.h"
#include "sot_collider.h"

typedef struct sot_tilemap_t {
    cute_tiled_map_t *tilemap;
    sot_collider_node_t *colliders;
    SDL_Texture *tilesetTexture;
} sot_tilemap_t;


sot_tilemap_t *CreateTilemap(char *name, AppState *appState); 
void RenderTilemap(sot_tilemap_t *tilemap, AppState *appState);
void DestroyTilemap(sot_tilemap_t *tilemap);
sot_collider_t *GetCollider(cute_tiled_object_t *tiledObject) ;
cute_tiled_layer_t *GetLayer(cute_tiled_map_t *map, char *layerName);