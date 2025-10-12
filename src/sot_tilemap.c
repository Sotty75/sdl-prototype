#include <stdlib.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include "sot_tilemap.h"
#include "appstate.h"
#include "sot_texture.h"


sot_tilemap_t *CreateTilemap(char *tilemapName, AppState *appState) 
{
    char *tileMapPath = NULL;
    SDL_asprintf(&tileMapPath, "%sassets\\%s", SDL_GetBasePath(), tilemapName);
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(tileMapPath, NULL);
    int width = map->width;
    int height = map->height;

    //...load the atlas in the textures pool.
    char *tilesetImage = (char *)map->tilesets->image.ptr;
    SDL_Texture *tileset = GetTexture(tilesetImage, appState);
    sot_tilemap_t *sot_tilemap = malloc(sizeof(sot_tilemap_t));
    sot_tilemap->tilemap = map;
    sot_tilemap->tilesetTexture = tileset;


    return sot_tilemap;
}

void RenderTilemap(sot_tilemap_t *current_tilemap, AppState *appState) {

    //...render the map
    cute_tiled_map_t *map = current_tilemap->tilemap;

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int currentTile = x + map->width * y;
            int size = map->layers[0].data_count;
            int currentTileIndex = map->layers[0].data[currentTile];

            // tiles with index 0 are empty, move to the next one
            if (currentTileIndex == 0)
                continue;

            // we need to decrement the tile index to have a zero starting index.
            currentTileIndex--;

            // source rectangle depends in the currentTileIndex
            // i need also to know the size of the tileset in terms of width and height and the size of each tile
            int tileWidth = map->tilesets[0].tilewidth;
            int tileHeight = map->tilesets[0].tileheight;
            int tilesetWidth = map->tilesets[0].imagewidth;
            int tilesetHeight = map->tilesets[0].imageheight;

            SDL_FRect sourceTile = 
            {   
                .x = (currentTileIndex * tileWidth) % tilesetWidth,
                .y = tileHeight * ((currentTileIndex * tileWidth) / tilesetWidth),
                .w = tileWidth,
                .h = tileHeight
            };

            // destination rectangle are the tiles on the screen.
            // The tiled map should already containing this information
            //      Tiles per row
            //      Tiles per column
            // in this case we move from the top to the bottom, we are already in the cycle, so we have both X and Y.
            SDL_FRect destinationTile = {
                .x = (x % map->width) * tileWidth,
                .y = y * tileHeight,
                .w = tileWidth,
                .h = tileHeight
            };

            SDL_RenderTexture(appState->renderer, current_tilemap->tilesetTexture, &sourceTile, &destinationTile);
        }
    }
}

void DestroyTilemap(sot_tilemap_t *current_tilemap) {
    cute_tiled_free_map(current_tilemap->tilemap);
    
    free(current_tilemap);
}