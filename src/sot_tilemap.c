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
    SDL_asprintf(&tileMapPath, "%sassets\\maps\\%s", SDL_GetBasePath(), tilemapFilename);
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

/// @brief This function performs two different operations. 
///
/// 1. Initialize the data structures to be sent to the GPU specific for the Tilemap.
///
/// 2. Upload the data to the GPU RAM.
/// 
/// @param current_tilemap In-Memory object model of the tiled map.
/// @param gpuData Generid data stracture to store GPU source data (vertex, index, storage buffers data and textures.)
void SOT_GPU_InitializeTilemap(sot_tilemap *tm, SOT_GPU_State *gpu) {

    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};
    gpuData.pipelineID = SOT_RP_TILEMAP;

    // Create a new quad to use as a template for the single tile
    sot_quad tilemapQuad = sot_quad_create();

    // Vertext Buffer Data
    gpuData.vertexDataSize = QUAD_VERTS * sizeof(vertex);
    gpuData.vertexData = (vertex *) malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, tilemapQuad.verts, gpuData.vertexDataSize);

    // Index Buffer Data
    gpuData.indexDataSize = QUAD_INDEXES * sizeof(uint16_t);
    gpuData.indexData = (uint16_t *) malloc(gpuData.indexDataSize);
    memcpy(gpuData.indexData, tilemapQuad.indexes, gpuData.indexDataSize);

    // Textures Data
    SDL_Surface *tilesetSurface = NULL;
    GetSurfaceFromImage(&tilesetSurface, "textures", tm->tilesetFilename);
    gpuData.surfaces[0] = tilesetSurface;
    gpuData.surfaceCount = 1;

    // Load Tilemap Data
    gpuData.tilemapDataSize = tm->tilesCount * (sizeof(int));
    gpuData.tilemapData = (int *) malloc(gpuData.tilemapDataSize);
    SDL_memcpy(gpuData.tilemapData, tm->tiles, gpuData.tilemapDataSize);

    //...upload data to GPU buffers used by the shader
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX | SOT_BUFFER_INDEX | SOT_BUFFER_TEXTURE | SOT_TILEMAP_SSB);
}

// Render the tilemap
// - Initialize the tilemapInfo struct to pass to the shader as a uniform
// - Push tilemapInfo struct as a unifotm
// - Push the projection view as a uniform
// - Bind Vertex and Index buffers for the basic tile QUAD
// - render the tiles as one instance x tile
void SOT_GPU_RenderTilemap(sot_tilemap *tm, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix)
{
    SDL_GPUTextureSamplerBinding textureBindings[gpu->buffers[SOT_RP_TILEMAP].texturesCount];
    for (int i = 0; i < gpu->buffers[SOT_RP_TILEMAP].texturesCount; i++ ) {
        textureBindings[i] = (SDL_GPUTextureSamplerBinding) {
            .texture = gpu->buffers[SOT_RP_TILEMAP].textures[i], 
            .sampler = gpu->nearestSampler
        };
    }
    SDL_BindGPUGraphicsPipeline(rpi->renderpass, gpu->pipeline[SOT_RP_TILEMAP]);
    SDL_BindGPUFragmentSamplers(rpi->renderpass, 0, textureBindings, gpu->buffers[SOT_RP_TILEMAP].texturesCount);
    SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 0, pvMatrix, sizeof(mat4));
    SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 1, &(tm->gpuTilemapInfo), sizeof(SOT_GPU_TilemapInfo));
    SDL_BindGPUVertexBuffers(rpi->renderpass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->buffers[SOT_RP_TILEMAP].vertexBuffer, .offset = 0}, 1);
    SDL_BindGPUIndexBuffer(rpi->renderpass, &(SDL_GPUBufferBinding) {.buffer = gpu->buffers[SOT_RP_TILEMAP].indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_BindGPUVertexStorageBuffers(rpi->renderpass, 0, gpu->buffers[SOT_RP_TILEMAP].storageBuffer, 1);

    // Draw all the tiles of the shader
    SDL_DrawGPUIndexedPrimitives(rpi->renderpass, 6, tm->tilesCount, 0, 0, 0);

    if (gpu->pipelineFlags & SOT_RPF_DEBUG)
    {
        SOT_GPU_ClearLines(gpu);

        // Colliders Debug Info
        sot_collider_node_t * collider = tm->colliders;
        while (collider) {
           DrawCollidersDebugInfo(gpu, *(collider->collider));
           collider = collider->next;
        }
        // Test Data to Validate the debug info renderer
        
        SOT_GPU_UploadDebugInfo(gpu);
        SOT_GPU_RenderDebugInfo(gpu, rpi, pvMatrix);
    }
        
    


    
       
}

void SOT_GPU_UpdateTilemapDebugInfo(sot_tilemap *current_tilemap, SOT_GPU_State* gpu) 
{

}


void DestroyTilemap(sot_tilemap *current_tilemap) {
    cute_tiled_free_map(current_tilemap->tilemap);
    DestroyColliders(current_tilemap->colliders);
    free(current_tilemap);
}