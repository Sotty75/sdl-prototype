#ifndef SOT_SCENE_H_
#define SOT_SCENE_H_

#include <stdlib.h>
#include "appstate.h"
#include "sot_camera.h"
#include "sot_actor.h"
#include "sot_animation.h"
#include "sot_tilemap.h"
#include "cute_tiled.h"
#include  "sot_gpu_pipeline.h"

// Actor spawn definition parsed from scene descriptor JSON
typedef struct SOT_ActorSpawnDef {
    char templateName[64];
    char instanceName[64];
    char spawnPoint[64];    // Named spawn point from Tiled map (or empty for explicit pos)
    float posX, posY;       // Explicit position (used if spawnPoint is empty)
} SOT_ActorSpawnDef;

typedef struct SOT_SceneDescriptor {
    int id;
    char *map;
    char *spritesheet[16];
    int spritesheetCount;
    SOT_ActorSpawnDef actorSpawns[64];
    int actorSpawnCount;

    // Camera config (loaded from scene JSON)
    bool hasCameraConfig;
    SOT_CameraFollow cameraFollow;
    int cameraTargetActor;
} SOT_SceneDescriptor;

typedef struct SOT_Scene {
    int id;
    char name[64];
    int actorsCount;
    SOT_Actor actors[2000];
    SOT_GPU_SpriteInstance gpuSpritesInfo[2000];
    sot_tilemap *tilemap;
    sot_camera worldCamera;
    sot_camera uiCamera;
    SOT_CameraFollow cameraFollow;
    int cameraTargetActor;      // Actor index to follow (-1 = none)
    SOT_PhysicsWorld physics;
    SOT_SceneDescriptor descriptor; // Stored for round-trip save (step 8)
    bool editorShowTilemap;     // Editor visibility toggle
    bool editorShowDebug;       // Editor debug overlay toggle
} SOT_Scene;

// ---- Scene manager ----
typedef struct SOT_SceneManager {
    char pendingScene[128];     // Scene to load next frame (empty = none)
    bool transitionActive;
    float transitionTimer;
    float transitionDuration;
} SOT_SceneManager;


SOT_SceneDescriptor SOT_LoadScene(char *sceneName);
SOT_Scene* SOT_InitializeScene(AppState *as, char *sceneName);
bool SOT_SaveScene(const SOT_Scene *scene, const char *sceneName);
void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime);

// Scene manager
void SOT_SceneManager_Init(SOT_SceneManager *mgr);
void SOT_SceneManager_RequestLoad(SOT_SceneManager *mgr, const char *sceneName, float transitionDuration);
bool SOT_SceneManager_HasPending(const SOT_SceneManager *mgr);

// Rendering section - Tilemap
void SOT_GPU_InitializeTilemap(sot_tilemap *current_tilemap, SOT_GPU_State *gpu);
void SOT_GPU_RenderTilemap(sot_tilemap *current_tilemap, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix);

// Rendering section - Actors
void SOT_GPU_InitializeActors(SOT_Scene *scene, SOT_GPU_State *gpu);
void SOT_GPU_RenderActors(SOT_Scene *scene, SOT_GPU_State* gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix);

void SOT_GPU_RenderScene(SOT_Scene * scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi);
void DestroyScene(SOT_Scene * scene);

#endif
