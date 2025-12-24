#include <stdlib.h>

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include "appstate.h"
#include "sot_gpu_pipeline.h"
#include "sot_tilemap.h"
#include "sot_texture.h"


sot_tilemap_t *CreateTilemap(char *tilemapFilename, AppState *appState) 
{
    // Allocate memory for the tilemap object
    sot_tilemap_t *tm = malloc(sizeof(sot_tilemap_t));

    // Load the tilemap from the file
    char *tileMapPath = NULL;
    SDL_asprintf(&tileMapPath, "%sassets\\%s", SDL_GetBasePath(), tilemapFilename);
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(tileMapPath, NULL);

    //...fill the tilemap object properties
    tm->tilesetFilename = (char *)map->tilesets->image.ptr;
    tm->tilemap = map;
    tm->tilesCount =  map->layers[0].data_count;
    int dataSize = tm->tilesCount * sizeof(int);
    tm->tiles = (int *)malloc(dataSize);
    SDL_memcpy(tm->tiles,  map->layers[0].data, dataSize);

    //...fill the gpu tilemap info
    tm->gpuTilemapInfo.COLUMNS = map->layers[0].height;
    tm->gpuTilemapInfo.TILE_WIDTH = map->tilesets[0].tilewidth;
    tm->gpuTilemapInfo.TILE_HEIGHT = map->tilesets[0].tileheight;
    tm->gpuTilemapInfo.TILESET_WIDTH = map->tilesets[0].imagewidth;

    //...load the colliders from the colliders layer
    cute_tiled_layer_t *collidersLayer = GetLayer(map, "Game-Collisions");
    cute_tiled_object_t *currentObject = collidersLayer->objects;

    //...fill the colliders list
    sot_collider_node_t *previousNode = NULL;
    
    while (currentObject != NULL) {
        sot_collider_node_t *currentNode = malloc(sizeof(sot_collider_node_t));
        currentNode->collider = GetCollider(currentObject);
        currentNode->next = NULL;
        if (previousNode == NULL) { tm->colliders = currentNode; }
        else { previousNode->next = currentNode; }
        previousNode = currentNode;

        currentObject = currentObject->next;
    }

    // Add the list of tilemap colliders to the linked list of world colliders
    AppendCollidersList(&(appState->pStaticColliders), tm->colliders);

    return tm;
}

sot_collider_t *GetCollider(cute_tiled_object_t *tiledObject) {
    // get collider type
    char *colliderType = NULL;
    cute_tiled_property_t *properties = tiledObject->properties;
    for (int i=0; i < tiledObject->property_count; ++i) {
        if (strcmp(properties[i].name.ptr, "colliderType") == 0) {
            colliderType = (char *)properties[i].data.string.ptr;
            break;
        }
    }

    if (colliderType == NULL) return NULL;

    // create collider and return
    sot_collider_t *collider = malloc(sizeof(sot_collider_t));
    memset(collider, 0, sizeof(collider));
    
    if (strcmp(colliderType, "AABB") == 0) {
        collider->type = C2_TYPE_AABB; 
        collider->shape.AABB.min.x = tiledObject->x;
        collider->shape.AABB.min.y = tiledObject->y;
        collider->shape.AABB.max.x = tiledObject->x + tiledObject->width;
        collider->shape.AABB.max.y = tiledObject->y + tiledObject->height;
    }
    else if (strcmp(colliderType, "POLY") == 0) {
        
        collider->type = C2_TYPE_POLY;
        collider->shape.poly.count = tiledObject->vert_count;
        
        for (int i = 0; i < tiledObject->vert_count; i++) {
            collider->shape.poly.verts[i].x = tiledObject->x + tiledObject->vertices[2*i];
            collider->shape.poly.verts[i].y = tiledObject->y + tiledObject->vertices[2*i+1];
        }

        // Generates the normals information.
        c2MakePoly(&(collider->shape.poly));
    }
    
    return collider;
}


cute_tiled_layer_t *GetLayer(cute_tiled_map_t *map, char *layerName) {
    cute_tiled_layer_t *currentLayer = map->layers;
    while (currentLayer != NULL) {
        if (strcmp(layerName, currentLayer->name.ptr) == 0) 
            return currentLayer;
        currentLayer = currentLayer->next;
    }

    return currentLayer;
}

void RenderTilemap(sot_tilemap_t *current_tilemap, AppState *appState) {

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

            // SDL_RenderTexture(appState->gpu->renderer, current_tilemap->tilesetTexture, &sourceTile, &destinationTile);
        }

        sot_collider_node_t * collider = current_tilemap->colliders;
            while (collider) {
                DrawCollidersDebugInfo(*(collider->collider), appState);
                collider = collider->next;
            }
    }
}

void DestroyTilemap(sot_tilemap_t *current_tilemap) {
    cute_tiled_free_map(current_tilemap->tilemap);
    DestroyColliders(current_tilemap->colliders);
    free(current_tilemap);
}