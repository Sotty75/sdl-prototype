#define CUTE_TILED_IMPLEMENTATION

#include "common.h"
#include "sot_scene.h"

// Create all the actors
// Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
// player, later we will use a file as an input (JSON, XML....)
SOT_Scene *CreateScene(AppState *as) {

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

    if (true == false) 
    {
        //... load the spritesheet inside of the texture
        SDL_Surface *monkeySpriteSheet = NULL;
        SOT_Animation *walkRight = NULL;
        SOT_Animation *walkLeft = NULL;
        SOT_Animation *idle = NULL;
        walkRight = CreateAnimation("Monkey_WalkRight", monkeySpriteSheet, 0, 8, 16, 16, 75, true, as);
        walkLeft = CreateAnimation("Monkey_WalkLeft", monkeySpriteSheet, 9, 17, 16, 16, 75, true, as);
        idle = CreateAnimation("Monkey_Idle", monkeySpriteSheet, 18, 23, 16, 16, 75, true, as);

        //...pack the animations in a NULL terminated array
        SOT_Animation **animations = malloc(sizeof(SOT_Animation*) * 4);
        animations[0] = idle;
        animations[1] = walkLeft;
        animations[2] = walkRight;
        animations[3] = NULL;

        // Create the player actor and add it to the scene.
        // ...get the player start position from the marker in the tiled-map
        cute_tiled_object_t *player = scene->tilemap->tilemap->layers[1].objects;
        cute_tiled_object_t *playerObject = SOT_GetObjectByName(scene->tilemap->tilemap, "player_start");
        vec2 startPosition = { playerObject->x, playerObject->y };
        SOT_Actor *player = CreateActor(as, "Player", startPosition, animations, C2_TYPE_CIRCLE);
        scene->player = player;

    }
    
    return scene;
}

void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status
    // UpdateActor(as, scene->player, deltaTime);
    UpdateCameraPan(&scene->worldCamera, (vec3) {0,0,0}, deltaTime, 50);
    return;
}

void SOT_GPU_RenderScene(SOT_Scene *scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi)
{
    SOT_GPU_RenderTilemap(scene->tilemap, gpu, rpi, scene->worldCamera.pvMatrix);

    // ------------------------------------------------- Render Actors Section ----------------------------------------------------------//
    // ------------------------------------------------- Render UI Section --------------------------------------------------------------//    
}

void DestroyScene(SOT_Scene * scene) {
    DestroyActor(scene->player);
    DestroyTilemap(scene->tilemap);
    free(scene);
}