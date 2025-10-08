#include <stdlib.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include "sot_tilemap.h"
#include "appstate.h"
#include "sot_texture.h"


SOT_Tilemap *CreateTilemap(char *tilemapName, AppState *appState) 
{
    char *tileMapPath = NULL;
    SDL_asprintf(&tileMapPath, "%sassets\\%s", SDL_GetBasePath(), tilemapName);
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(tileMapPath, NULL);
    int width = map->width;
    int height = map->height;

    //...load the atlas in the textures pool.
    char *tilesetPath = NULL;
    SDL_asprintf(&tilesetPath, "%sassets\\%s", SDL_GetBasePath(), map->tilesets->name.ptr);
    SDL_Texture *tileset = GetTexture(tilesetPath, appState);

    SOT_Tilemap *sot_tilemap = malloc(sizeof(SOT_Tilemap));
    sot_tilemap->tilemap = map;
    sot_tilemap->tilesetTexture = tileset;


    return sot_tilemap;
}

void RenderTilemap(SOT_Tilemap *current_tilemap, AppState *appState) {

    //...render the map
    cute_tiled_map_t *map = current_tilemap->tilemap;

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            // get tile index from the map
            // calculate render position
            // 
        }
    }
}

void DestroyTilemap(SOT_Tilemap *current_tilemap) {
    cute_tiled_free_map(current_tilemap->tilemap);
    
    free(current_tilemap);
}