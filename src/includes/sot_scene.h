#ifndef SCENE_H_
#define SCENE_H_

#include <stdlib.h>
#include "appstate.h"
#include "sot_camera.h"
#include "sot_actor.h"
#include "sot_animation.h"
#include "sot_tilemap.h"
#include "cute_tiled.h"
#include  "sot_gpu_pipeline.h"

typedef struct SOT_Scene {
    int id;
    int actorsCount;
    SOT_Actor actors[2000];
    SOT_GPU_SpriteInstance gpuSpritesInfo[2000];
    sot_tilemap *tilemap;
    sot_camera worldCamera;
    sot_camera uiCamera;
} SOT_Scene;


SOT_Scene* SOT_InitializeScene(AppState *as);
void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime);

// Rendering section - Tilemap
void SOT_GPU_InitializeTilemap(sot_tilemap *current_tilemap, SOT_GPU_State *gpu);
void SOT_GPU_RenderTilemap(sot_tilemap *current_tilemap, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix);

// Rendering section - Actors
void SOT_GPU_InitializeActors(SOT_Scene *scene, SOT_GPU_State *gpu);
void SOT_GPU_RenderActors(SOT_Scene *scene, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix);

void SOT_GPU_RenderScene(SOT_Scene * scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi);
void DestroyScene(SOT_Scene * scene);

#endif
