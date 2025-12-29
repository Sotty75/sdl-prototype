#ifndef SCENE_H_
#define SCENE_H_

#include <stdlib.h>
#include "appstate.h"
#include "sot_camera.h"
#include "sot_actor.h"
#include "sot_sprite.h"
#include "sot_tilemap.h"
#include "cute_tiled.h"
#include  "sot_gpu_pipeline.h"

typedef struct SOT_Scene {
    int id;
    sot_actor_t *player;
    sot_tilemap *sot_tilemap;
    sot_camera worldCamera;
    sot_camera uiCamera;
} SOT_Scene;


SOT_Scene* CreateScene(AppState *as);
void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime);
void SOT_GPU_RenderScene(SOT_Scene * scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi);
void DestroyScene(SOT_Scene * scene);

#endif
