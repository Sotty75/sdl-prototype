#include <stdlib.h>

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include "appstate.h"
#include "sot_tilemap.h"
#include "sot_texture.h"
#include "sot_gpu_pipeline.h"
#include "sot_quad.h"


sot_tilemap *SOT_CreateTilemap(char *tilemapFilename, AppState *appState) 
{
    // Allocate memory for the tilemap object
    sot_tilemap *tm = malloc(sizeof(sot_tilemap));

    // Load the tilemap from the file
    char *tileMapPath = NULL;
    SDL_asprintf(&tileMapPath, "%s\\%s", Paths.TiledMaps, tilemapFilename);
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(tileMapPath, NULL);

    //...fill the tilemap object properties
    tm->tilesetFilename = (char *)map->tilesets->image.ptr;
    tm->tilemap = map;
    tm->tilesCount =  map->layers[0].data_count;
    int dataSize = tm->tilesCount * sizeof(int);
    tm->tiles = (int *)malloc(dataSize);
    SDL_memcpy(tm->tiles,  map->layers[0].data, dataSize);

    //...fill the gpu tilemap info
    tm->gpuTilemapInfo.COLUMNS = map->layers[0].width;
    tm->gpuTilemapInfo.ROWS = map->layers[0].height;
    tm->gpuTilemapInfo.TILE_WIDTH = map->tilesets[0].tilewidth;
    tm->gpuTilemapInfo.TILE_HEIGHT = map->tilesets[0].tileheight;
    tm->gpuTilemapInfo.TILESET_WIDTH = map->tilesets[0].imagewidth;
    tm->gpuTilemapInfo.TILESET_HEIGHT = map->tilesets[0].imageheight;

    //...load the colliders from the colliders layer
    cute_tiled_layer_t *collidersLayer = SOT_GetLayer(map, "Game-Collisions");
    cute_tiled_object_t *currentObject = collidersLayer->objects;

    //...fill the colliders list
    sot_collider_node_t *previousNode = NULL;
    
    while (currentObject != NULL) {
        sot_collider_node_t *currentNode = malloc(sizeof(sot_collider_node_t));
        currentNode->collider = SOT_GetCollider(currentObject);
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

sot_collider_t *SOT_GetCollider(cute_tiled_object_t *tiledObject) {
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
        collider->shape.AABB.min.y = -tiledObject->y;
        collider->shape.AABB.max.x = tiledObject->x + tiledObject->width;
        collider->shape.AABB.max.y = -(tiledObject->y + tiledObject->height);
    }
    else if (strcmp(colliderType, "POLY") == 0) {
        
        collider->type = C2_TYPE_POLY;
        collider->shape.poly.count = tiledObject->vert_count;
        
        for (int i = 0; i < tiledObject->vert_count; i++) {
            collider->shape.poly.verts[i].x = tiledObject->x + tiledObject->vertices[2*i];
            collider->shape.poly.verts[i].y = -(tiledObject->y + tiledObject->vertices[2*i+1]);
        }

        // Generates the normals information.
        c2MakePoly(&(collider->shape.poly));
    }
    
    return collider;
}


cute_tiled_layer_t *SOT_GetLayer(cute_tiled_map_t *map, char *layerName) {
    cute_tiled_layer_t *currentLayer = map->layers;
    while (currentLayer != NULL) {
        if (strcmp(layerName, currentLayer->name.ptr) == 0) 
            return currentLayer;
        currentLayer = currentLayer->next;
    }

    return currentLayer;
}

cute_tiled_object_t *SOT_GetObjectByName(cute_tiled_map_t *map, char *objectName) {
    cute_tiled_object_t *currentObject = map->layers[1].objects;
    while (currentObject != NULL) {
        if (strcmp(currentObject->name.ptr, objectName) == 0) {
            return currentObject;
        }
    }

    return NULL;
}




void DestroyTilemap(sot_tilemap *current_tilemap) {
    cute_tiled_free_map(current_tilemap->tilemap);
    DestroyColliders(current_tilemap->colliders);
    free(current_tilemap);
}