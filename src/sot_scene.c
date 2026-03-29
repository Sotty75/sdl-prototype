#define CUTE_TILED_IMPLEMENTATION

#include "sot_common.h"
#include "sot_scene.h"


SOT_SceneDescriptor SOT_LoadScene(char *sceneName) {

    // Initialize the scene descriptor structure
    SOT_SceneDescriptor sceneDesc = {0};

    // Load the scene fromt the scene descriptor file
    char *scenePath = NULL;
    SDL_asprintf(&scenePath, "%s\\%s", Paths.Scenes, sceneName);  
    
	//...read the file contents into a string
    FILE *fp = fopen(scenePath, "r");
    if (fp == NULL) {
        SDL_Log("Error: Unable to open the scene file %s.\n", scenePath);
        SDL_free(scenePath);
        return sceneDesc;
    }
    SDL_free(scenePath);
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    char *content = (char *)SDL_calloc(fileSize+1, 1);
    int len = fread(content, 1, fileSize, fp);
    content[fileSize] = '\0';
    fclose(fp);

	//...parse the JSON data
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);

        return sceneDesc;
    }
    
    int i = 0;
	cJSON *spritesheet = NULL;
    cJSON *spritesheets = cJSON_GetObjectItemCaseSensitive(json, "spritesheets");

	cJSON_ArrayForEach(spritesheet, spritesheets)
	{
        cJSON *nameItem = cJSON_GetObjectItemCaseSensitive(spritesheet, "name");
        if (nameItem == NULL || !cJSON_IsString(nameItem)) continue;
        char *fileName = nameItem->valuestring;
        size_t size = SDL_strlen(fileName)+1;
        sceneDesc.spritesheet[i] = (char *)SDL_malloc(size);
        SDL_strlcpy(sceneDesc.spritesheet[i], fileName, size);
        sceneDesc.spritesheetCount++;

        i++;
	}

    cJSON *idItem = cJSON_GetObjectItemCaseSensitive(json, "id");
    cJSON *mapItem = cJSON_GetObjectItemCaseSensitive(json, "map");
    if (idItem == NULL || !cJSON_IsNumber(idItem) || mapItem == NULL || !cJSON_IsString(mapItem)) {
        cJSON_Delete(json);
        SDL_free(content);
        return sceneDesc;
    }
    sceneDesc.id = idItem->valueint;
    char *mapName = mapItem->valuestring;
    size_t size = SDL_strlen(mapName)+1;
    sceneDesc.map = (char *)SDL_malloc(size);
    SDL_strlcpy(sceneDesc.map, mapName, size);
    

    cJSON_Delete(json);
    SDL_free(content);

    return sceneDesc;
}


// Create all the actors
// Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
// player, later we will use a file as an input (JSON, XML....)
SOT_Scene *SOT_InitializeScene(AppState *as, char *sceneName) {

    SOT_Scene *scene = SDL_malloc(sizeof(SOT_Scene));
    if (scene == NULL) return NULL;

    SOT_SceneDescriptor sceneDesc = SOT_LoadScene(sceneName);
    if (sceneDesc.id == 0)
        return NULL;

    // set the scene ID
    scene->id = sceneDesc.id;

    // ...create the tilemap and include it into the scene
    scene->tilemap = SOT_CreateTilemap(sceneDesc.map, as);
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
    scene->worldCamera = CreateCameraWithInfo(cameraInfo, projectionInfo);
    scene->uiCamera = CreateCameraWithInfo(cameraInfo, projectionInfo);

        

    // Create the player actor and add it to the scene.
    // Get the player start position from the marker in the tiled-map
    cute_tiled_object_t *playerObject = SOT_GetObjectByName(scene->tilemap->tilemap, "player_start");
    if (playerObject == NULL) {
        SDL_Log("Error: 'player_start' object not found in tilemap.");
        SDL_free(scene);
        return NULL;
    }
    vec2 startPosition = { playerObject->x, playerObject->y };
    scene->actors[0] = SOT_CreateActor(as, "Player", startPosition, "monkey.json");
    scene->actorsCount++;

    ///////////////////////////////////////////////////
    // TODO:: Load other actors in the scene if present
    ///////////////////////////////////////////////////

    if (as->gpu->pipelineFlags & SOT_RP_TILEMAP_FLAG)
        SOT_GPU_InitializeTilemap(scene->tilemap, as->gpu);

    if (as->gpu->pipelineFlags & SOT_RP_SPRITES_FLAG)
        SOT_GPU_InitializeActors(scene, as->gpu);

    return scene;
}

void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status
    UpdateActor(as, &scene->actors[0], deltaTime);
    UpdateCameraPan(&scene->worldCamera, (vec3) {0,0,0}, deltaTime, 50);

    // Populate GPU sprite instance data from each actor's current animation frame
    for (int i = 0; i < scene->actorsCount; i++) {
        SOT_Actor *actor = &scene->actors[i];
        SOT_Animation *anim = &actor->animations[actor->currentAnimation];
        SOT_AnimationSequence *seq = &anim->info->sequences[anim->sequenceIndex];
        vec4 *frame = &seq->frames[anim->currentFrame];

        scene->gpuSpritesInfo[i].position[0] = actor->transform.position[0];
        scene->gpuSpritesInfo[i].position[1] = actor->transform.position[1];
        scene->gpuSpritesInfo[i].frameCoords[0] = (int)(*frame)[0];
        scene->gpuSpritesInfo[i].frameCoords[1] = (int)(*frame)[1];
        scene->gpuSpritesInfo[i].frameSize[0]   = (int)(*frame)[2];
        scene->gpuSpritesInfo[i].frameSize[1]   = (int)(*frame)[3];
        scene->gpuSpritesInfo[i].atlasSize[0]   = anim->atlasSize[0];
        scene->gpuSpritesInfo[i].atlasSize[1]   = anim->atlasSize[1];
        scene->gpuSpritesInfo[i].atlasIndex     = anim->atlasIndex;
    }

    return;
}

void SOT_GPU_RenderScene(SOT_Scene *scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi)
{
    // ------------------------------------------------- Render Tilemap Section ----------------------------------------------------------//
    if (gpu->pipelineFlags & SOT_RP_TILEMAP_FLAG) {
        SOT_GPU_RenderTilemap(scene->tilemap, gpu, rpi, scene->worldCamera.pvMatrix);        
    }
    
    // ------------------------------------------------- Render Actors Section ----------------------------------------------------------//
    if (gpu->pipelineFlags & SOT_RP_SPRITES_FLAG) {
        SOT_GPU_RenderActors(scene, gpu, rpi, scene->worldCamera.pvMatrix);
    }
    
    // ------------------------------------------------- Render UI Section --------------------------------------------------------------//    
    if (gpu->pipelineFlags & SOT_RP_OVERLAY_FLAG) {
        
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

    if (gpu->pipelineFlags & SOT_RP_DEBUG_FLAG)
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
    gpuData.vertexData = (vertex *) SDL_malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, tilemapQuad.verts, gpuData.vertexDataSize);

    // Index Buffer Data
    gpuData.indexDataSize = QUAD_INDEXES * sizeof(uint16_t);
    gpuData.indexData = (uint16_t *) SDL_malloc(gpuData.indexDataSize);
    memcpy(gpuData.indexData, tilemapQuad.indexes, gpuData.indexDataSize);

    // Textures Data
    SDL_Surface *tilesetSurface = NULL;
    GetSurfaceFromImage(&tilesetSurface, tm->tilesetFilename);
    gpuData.surfaces[0] = tilesetSurface;
    gpuData.surfaceCount = 1;

    // Load Tilemap Data
    gpuData.tilemapDataSize = tm->tilesCount * (sizeof(int));
    gpuData.tilemapData = (int *) SDL_malloc(gpuData.tilemapDataSize);
    SDL_memcpy(gpuData.tilemapData, tm->tiles, gpuData.tilemapDataSize);

    //...upload data to GPU buffers used by the shader
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX | SOT_BUFFER_INDEX | SOT_BUFFER_TEXTURE | SOT_TILEMAP_SSB);

    SDL_free(gpuData.vertexData);
    SDL_free(gpuData.indexData);
    SDL_free(gpuData.tilemapData);
    SDL_DestroySurface(gpuData.surfaces[0]);
}


void SOT_GPU_InitializeActors(SOT_Scene *scene, SOT_GPU_State *gpu) {
    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};
    gpuData.pipelineID = SOT_RP_SPRITE;

    // Create a new quad to use as a template for the single tile
    sot_quad spriteQuad = sot_quad_create();

    // Vertext Buffer Data
    gpuData.vertexDataSize = QUAD_VERTS * sizeof(vertex);
    gpuData.vertexData = (vertex *) SDL_malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, spriteQuad.verts, gpuData.vertexDataSize);

    // Index Buffer Data
    gpuData.indexDataSize = QUAD_INDEXES * sizeof(uint16_t);
    gpuData.indexData = (uint16_t *) SDL_malloc(gpuData.indexDataSize);
    memcpy(gpuData.indexData, spriteQuad.indexes, gpuData.indexDataSize);

    /* Textures: Iterate through the actors in the scene and collect all the atlas files to build an array of surfaces.
     * The shader will be loaded with as many surfaces as needed. Each sprite stores the atlas index and size
     * for use in UpdateScene when building per-frame GPU sprite instance data. */
    char *textures[16] = {0};
    int textureSizes[16][2] = {0};
    int textureIndex = 0;
    for (int i = 0; i < scene->actorsCount; i++) {
        for (int j = 0; j < scene->actors[i].animationsCount; j++) {

            char *currentAtlas = scene->actors[i].animations[j].info->atlasName;

            // Check if the texture has already been added
            int existingIndex = -1;
            for (int t = 0; t < textureIndex; t++) {
                if (SDL_strcmp(textures[t], currentAtlas) == 0) {
                    existingIndex = t;
                    break;
                }
            }

            if (existingIndex >= 0) {
                scene->actors[i].animations[j].atlasIndex    = existingIndex;
                scene->actors[i].animations[j].atlasSize[0]  = textureSizes[existingIndex][0];
                scene->actors[i].animations[j].atlasSize[1]  = textureSizes[existingIndex][1];
                continue;
            }

            // Track new texture
            textures[textureIndex] = currentAtlas;

            // Build filename with extension and load the surface
            char atlasFilename[256];
            SDL_snprintf(atlasFilename, sizeof(atlasFilename), "%s.png", currentAtlas);
            SDL_Surface *spritesheetSurface = NULL;
            GetSurfaceFromImage(&spritesheetSurface, atlasFilename);
            gpuData.surfaces[textureIndex] = spritesheetSurface;
            gpuData.surfaceCount++;

            // Store atlas dimensions and index on the animation for use in UpdateScene
            textureSizes[textureIndex][0]                    = spritesheetSurface->w;
            textureSizes[textureIndex][1]                    = spritesheetSurface->h;
            scene->actors[i].animations[j].atlasIndex        = textureIndex;
            scene->actors[i].animations[j].atlasSize[0]      = spritesheetSurface->w;
            scene->actors[i].animations[j].atlasSize[1]      = spritesheetSurface->h;

            textureIndex++;
        }
    }

    //...upload data to GPU buffers used by the shader (SOT_SPRITES_SSB creates the storage buffer)
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX | SOT_BUFFER_INDEX | SOT_BUFFER_TEXTURE | SOT_SPRITES_SSB);

    SDL_free(gpuData.vertexData);
    SDL_free(gpuData.indexData);
    for (int i = 0; i < gpuData.surfaceCount; i++)
        SDL_DestroySurface(gpuData.surfaces[i]);
}

void SOT_GPU_RenderActors(SOT_Scene *scene, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix)
{
    int spriteDataSize = scene->actorsCount * sizeof(SOT_GPU_SpriteInstance);

    // Upload current frame's sprite instance data to the GPU storage buffer via a temporary transfer buffer.
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTransferBuffer *spritesTransferBuffer = SDL_CreateGPUTransferBuffer(gpu->device, &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = spriteDataSize
    });

    void *mappedData = SDL_MapGPUTransferBuffer(gpu->device, spritesTransferBuffer, false);
    SDL_memcpy(mappedData, scene->gpuSpritesInfo, spriteDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, spritesTransferBuffer);

    SDL_UploadToGPUBuffer(
        copyPass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = spritesTransferBuffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = gpu->buffers[SOT_RP_SPRITE].storageBuffer[0],
            .offset = 0,
            .size = spriteDataSize
        },
        false
    );

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
    SDL_ReleaseGPUTransferBuffer(gpu->device, spritesTransferBuffer);

    SDL_GPUTextureSamplerBinding textureBindings[gpu->buffers[SOT_RP_SPRITE].texturesCount];
    for (int i = 0; i < gpu->buffers[SOT_RP_SPRITE].texturesCount; i++) {
        textureBindings[i] = (SDL_GPUTextureSamplerBinding) {
            .texture = gpu->buffers[SOT_RP_SPRITE].textures[i],
            .sampler = gpu->nearestSampler
        };
    }

    SDL_BindGPUGraphicsPipeline(rpi->renderpass, gpu->pipeline[SOT_RP_SPRITE]);
    SDL_BindGPUFragmentSamplers(rpi->renderpass, 0, textureBindings, gpu->buffers[SOT_RP_SPRITE].texturesCount);
    SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 0, pvMatrix, sizeof(mat4));
    SDL_BindGPUVertexBuffers(rpi->renderpass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->buffers[SOT_RP_SPRITE].vertexBuffer, .offset = 0}, 1);
    SDL_BindGPUIndexBuffer(rpi->renderpass, &(SDL_GPUBufferBinding) {.buffer = gpu->buffers[SOT_RP_SPRITE].indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_BindGPUVertexStorageBuffers(rpi->renderpass, 0, gpu->buffers[SOT_RP_SPRITE].storageBuffer, 1);

    SDL_DrawGPUIndexedPrimitives(rpi->renderpass, 6, scene->actorsCount, 0, 0, 0);

    if (gpu->pipelineFlags & SOT_RP_DEBUG_FLAG)
    {
        // TODO: Debug info for sprites.
    }
}


void DestroyScene(SOT_Scene * scene) {
    // Clean up actors' owned animation data
    for (int i = 0; i < scene->actorsCount; i++) {
        DestroyActor(&scene->actors[i]);
    }
    DestroyTilemap(scene->tilemap);
    SDL_free(scene);
}