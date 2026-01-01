#ifndef SOT_TILEMAP_H_
#define SOT_TILEMAP_H_

#include <stdlib.h>
#include "appstate.h"
#include "cute_tiled.h"
#include "sot_quad.h"
#include "sot_collider.h"
#include "sot_gpu_pipeline.h"

// ---------------------------------- Tilemap Render Pipeline Info -----------------------------//


// Tilemap Information loaded as a uniform in the tilemap 
// vertex shader
// 
// - COLUMNS: Tilemap columns
// - TILE_WIDTH: width of a single tile in pixels.
// - TILE_HEIGHT: height of a single tile in pixels.
// - TILESET_WIDTH: width of the source tileset in pixels.
typedef struct SOT_GPU_TilemapInfo {
    int32_t COLUMNS;
    int32_t ROWS;
    int32_t TILE_WIDTH;
    int32_t TILE_HEIGHT;
    int32_t TILESET_WIDTH;
    int32_t TILESET_HEIGHT;
} SOT_GPU_TilemapInfo;

// ------------------------------------------- Tilemap Entity Info --------------------------------------------------------------//

// Data structure holding all the information required for the rendering of the 
//
// - tilemapFilenam: name of the tilemap file
// - tilesetFilenam: name of the tileset atlas image to be loaded in the GPU
// - tiles: array of integers corresponding to the tile index for each tile in the tileset
// - tilescount: sized in tiles of the full map
// - gpuTilemapInfo: data required to inizialize the GPU to render the tileset
// - tilemap: reference to the inner "cute" library tilemap entity
// - colliders: array of colliders loaded from the tilemap
typedef struct sot_tilemap {
    char *tilemapFilename;
    char *tilesetFilename;
    int *tiles;
    int tilesCount;
    cute_tiled_map_t *tilemap;
    sot_collider_node_t *colliders;
    SOT_GPU_TilemapInfo gpuTilemapInfo;
} sot_tilemap;


// Entity management section
sot_tilemap *SOT_CreateTilemap(char *tilemapFilename, AppState *appState); 
void DestroyTilemap(sot_tilemap *tilemap);
sot_collider_t *SOT_GetCollider(cute_tiled_object_t *tiledObject) ;
cute_tiled_layer_t *SOT_GetLayer(cute_tiled_map_t *map, char *layerName);

// Rendering section
void SOT_GPU_InitializeTilemap(sot_tilemap *current_tilemap, SOT_GPU_State *gpu);
void SOT_GPU_RenderTilemap(sot_tilemap *current_tilemap, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix);
void SOT_GPU_UpdateTilemapDebugInfo(sot_tilemap *current_tilemap, SOT_GPU_State* gpu);


#endif