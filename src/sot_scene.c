#define CUTE_TILED_IMPLEMENTATION

#include "sot_common.h"
#include "sot_scene.h"

// Create all the actors
// Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
// player, later we will use a file as an input (JSON, XML....)
SOT_Scene *SOT_InitializeScene(AppState *as) {

    SOT_Scene *scene = malloc(sizeof(SOT_Scene));
    if (scene == NULL) return NULL;

    // set the scene ID
    scene->id = 1;

    // ...create the tilemap and include it into the scene
    char *assetName = "level_00.json";
    scene->tilemap = SOT_CreateTilemap(assetName, as);
    if (scene->tilemap == NULL) return NULL;

    // calculate the z-camera distance (fov / 2)
    float fov = 45.0;
    float zCamera = (1/SDL_tan(fov/2))*(0.5*(scene->tilemap->gpuTilemapInfo.ROWS + 1))*scene->tilemap->gpuTilemapInfo.TILE_HEIGHT;
    // set the camera at the center of the scene.    
    vec2 cameraPosition = {0.5*scene->tilemap->gpuTilemapInfo.COLUMNS*scene->tilemap->gpuTilemapInfo.TILE_WIDTH, -0.5*scene->tilemap->gpuTilemapInfo.ROWS*scene->tilemap->gpuTilemapInfo.TILE_HEIGHT};

    /* Create the cameras list entity */
    SOT_CameraInfo cameraInfo = {
        .center = {cameraPosition[0], cameraPosition[1], 0},
        .eye = {cameraPosition[0], cameraPosition[1],-zCamera},
        .up = {0,1,0}
    };
    SOT_ProjectionInfo projectionInfo = {
        .aspect = SCREEN_WIDTH / SCREEN_HEIGHT,
        .far = 100,
        .fov = 45,
        .near = 5,
        .mode = SOT_PERSPECTIVE,
    };
    scene->worldCamera = CreateCameraWitInfo(cameraInfo, projectionInfo);
    scene->uiCamera = CreateCameraWitInfo(cameraInfo, projectionInfo);

        

    // Create the player actor and add it to the scene.
    // Get the player start position from the marker in the tiled-map
    cute_tiled_object_t *playerObject = SOT_GetObjectByName(scene->tilemap->tilemap, "player_start");
    vec2 startPosition = { playerObject->x, playerObject->y };
    scene->actors[0] = SOT_CreateActor(as, "Player", startPosition, "monkey.json");
    scene->actorsCount++;

    ///////////////////////////////////////////////////
    // TODO:: Load other actors in the scene if present
    ///////////////////////////////////////////////////

    if (as->gpu->pipelineFlags & SOT_RPF_TILEMAP)
        SOT_GPU_InitializeTilemap(scene->tilemap, as->gpu);

    if (as->gpu->pipelineFlags & SOT_RPF_SPRITES)
        SOT_GPU_InitializeActors(scene, as->gpu);

    return scene;
}

void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status
    UpdateActor(as, &scene->actors[0], deltaTime);
    UpdateCameraPan(&scene->worldCamera, (vec3) {0,0,0}, deltaTime, 50);
    return;
}

void SOT_GPU_RenderScene(SOT_Scene *scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi)
{
    // ------------------------------------------------- Render Tilemap Section ----------------------------------------------------------//
    if (gpu->pipelineFlags & SOT_RPF_TILEMAP) {
        SOT_GPU_RenderTilemap(scene->tilemap, gpu, rpi, scene->worldCamera.pvMatrix);        
    }
    
    // ------------------------------------------------- Render Actors Section ----------------------------------------------------------//
    if (gpu->pipelineFlags & SOT_RPF_SPRITES) {
        
    }
    
    // ------------------------------------------------- Render UI Section --------------------------------------------------------------//    
    if (gpu->pipelineFlags & SOT_RPF_OVERLAY) {
        
    }
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


//////////////////////////////////////////////////////////////
// Rendering Pipelines - Functions to send scene data to the GPU
//////////////////////////////////////////////////////////////

/// @brief This function performs two different operations. 
///
/// 1. Initialize the data structures to be sent to the GPU specific for the Tilemap.
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
    GetSurfaceFromImage(&tilesetSurface, tm->tilesetFilename);
    gpuData.surfaces[0] = tilesetSurface;
    gpuData.surfaceCount = 1;

    // Load Tilemap Data
    gpuData.tilemapDataSize = tm->tilesCount * (sizeof(int));
    gpuData.tilemapData = (int *) malloc(gpuData.tilemapDataSize);
    SDL_memcpy(gpuData.tilemapData, tm->tiles, gpuData.tilemapDataSize);

    //...upload data to GPU buffers used by the shader
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX | SOT_BUFFER_INDEX | SOT_BUFFER_TEXTURE | SOT_TILEMAP_SSB);
}


void SOT_GPU_InitializeActors(SOT_Scene *scene, SOT_GPU_State *gpu) {
    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};
    gpuData.pipelineID = SOT_RP_SPRITE;

    // Create a new quad to use as a template for the single tile
    sot_quad spriteQuad = sot_quad_create();

    // Vertext Buffer Data
    gpuData.vertexDataSize = QUAD_VERTS * sizeof(vertex);
    gpuData.vertexData = (vertex *) malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, spriteQuad.verts, gpuData.vertexDataSize);

    // Index Buffer Data
    gpuData.indexDataSize = QUAD_INDEXES * sizeof(uint16_t);
    gpuData.indexData = (uint16_t *) malloc(gpuData.indexDataSize);
    memcpy(gpuData.indexData, spriteQuad.indexes, gpuData.indexDataSize);

    /* Textures: Iterate through the actors in the scene and collect all the atlas files to build an array of surfaces.
     * The shader will be loaded with as many surfaces as needed. Each sprite will hence have to store a 
     * reference to the texture index */
    char *textures[16] = {0};
    int textureIndex = 0;
    for (int i = 0; i < scene->actorsCount; i++) {
        for (int j = 0; j < scene->actors[i].animationsCount; j++) {

            char *currentAtlas = scene->actors[i].animations[j].atlasName;

            // Check if the texture has already been added
            int existingIndex = -1;
            for (int t = 0; t < textureIndex; t++) {
                if (SDL_strcmp(textures[t], currentAtlas) == 0) {
                    existingIndex = t;
                    break;
                }
            }

            if (existingIndex >= 0) {
                continue;
            }

            // Track new texture
            textures[textureIndex] = currentAtlas;

            // Include surface in GPU Data
            SDL_Surface *spritesheetSurface = NULL;
            GetSurfaceFromImage(&spritesheetSurface, currentAtlas);
            gpuData.surfaces[textureIndex] = spritesheetSurface;
            gpuData.surfaceCount++;

            textureIndex++;
        }
    }

    //...upload data to GPU buffers used by the shader
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX | SOT_BUFFER_INDEX | SOT_BUFFER_TEXTURE);
}

void SOT_GPU_RenderActors(SOT_Scene *scene, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix) 
{
    ///////////////  TODO: Move into the update actors section  //////////////////////

    // Get a command buffer for the copy pass.
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpu->device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);
    SDL_GPUTransferBuffer *spritesTransferBuffer = SDL_CreateGPUTransferBuffer(gpu->device, &(SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = 2000 * sizeof(SOT_GPU_SpriteInstance)
    });
    SDL_GPUBuffer *spritesBuffer = SDL_CreateGPUBuffer(gpu->device, &(SDL_GPUBufferCreateInfo){
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = 2000 * sizeof(SOT_GPU_SpriteInstance),
    });

    SDL_MapGPUTransferBuffer(gpu->device, spritesTransferBuffer, false);
    SDL_memcpy(spritesTransferBuffer, scene->gpuSpritesInfo, 2000*sizeof(SOT_GPU_SpriteInstance));
    SDL_UnmapGPUTransferBuffer(gpu->device, spritesTransferBuffer);
    
    SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = gpu->transferBuffers.storageTransferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = gpu->buffers[SOT_RP_SPRITE].storageBuffer[0],
			.offset = 0,
			.size =  2000 * sizeof(SOT_GPU_SpriteInstance)
		},
		false
    );

    SDL_GPUTextureSamplerBinding textureBindings[gpu->buffers[SOT_RP_SPRITE].texturesCount];
    for (int i = 0; i < gpu->buffers[SOT_RP_SPRITE].texturesCount; i++ ) {
        textureBindings[i] = (SDL_GPUTextureSamplerBinding) {
            .texture = gpu->buffers[SOT_RP_SPRITE].textures[i], 
            .sampler = gpu->nearestSampler
        };
    }
    //////////////////// End of update section ///////////////////

    SDL_BindGPUGraphicsPipeline(rpi->renderpass, gpu->pipeline[SOT_RP_SPRITE]);
    SDL_BindGPUFragmentSamplers(rpi->renderpass, 0, textureBindings, gpu->buffers[SOT_RP_SPRITE].texturesCount);
    SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 0, pvMatrix, sizeof(mat4));
    SDL_BindGPUVertexBuffers(rpi->renderpass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->buffers[SOT_RP_SPRITE].vertexBuffer, .offset = 0}, 1);
    SDL_BindGPUIndexBuffer(rpi->renderpass, &(SDL_GPUBufferBinding) {.buffer = gpu->buffers[SOT_RP_SPRITE].indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_BindGPUVertexStorageBuffers(rpi->renderpass, 0, gpu->buffers[SOT_RP_SPRITE].storageBuffer, 1);

    // Draw all the sprites of the scene
    SDL_DrawGPUIndexedPrimitives(rpi->renderpass, 6, scene->actorsCount, 0, 0, 0);
    
    if (gpu->pipelineFlags & SOT_RPF_DEBUG)
    {
        // TODO::Debug info for sprites.
    }
}


void DestroyScene(SOT_Scene * scene) {
    DestroyTilemap(scene->tilemap);
    free(scene);
}