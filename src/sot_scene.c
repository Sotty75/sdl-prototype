#define CUTE_TILED_IMPLEMENTATION

#include "common.h"
#include "sot_scene.h"

// Create all the actors
// Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
// player, later we will use a file as an input (JSON, XML....)
SOT_Scene *CreateScene(AppState *as) {

    SOT_Scene *currentScene = malloc(sizeof(SOT_Scene));
    if (currentScene == NULL) return NULL;

    // set the scene ID
    currentScene->id = 1;

    // ...create the tilemap and include it into the scene
    char *assetName = "level_00.json";
    currentScene->sot_tilemap = SOT_CreateTilemap(assetName, as);
    if (currentScene->sot_tilemap == NULL) return NULL;


    /* Create the cameras list entity */
    SOT_CameraInfo cameraInfo = {
        .center = {0, 0, 0},
        .eye = {0,0,-1},
        .up = {0,1,0}
    };
    SOT_ProjectionInfo projectionInfo = {
        .aspect = SCREEN_WIDTH / SCREEN_HEIGHT,
        .far = 100,
        .fov = 45,
        .near = 5,
        .mode = SOT_PERSPECTIVE,
    };
    currentScene->worldCamera = CreateCameraWitInfo(cameraInfo, projectionInfo);
    currentScene->uiCamera = CreateCameraWitInfo(cameraInfo, projectionInfo);

    if (true == false) 
    {
        //... load the spritesheet inside of the texture
        SDL_Surface *monkeySpriteSheet = NULL;
        sot_sprite_t *walkRight = NULL;
        sot_sprite_t *walkLeft = NULL;
        sot_sprite_t *idle = NULL;
        walkRight = CreateAnimation("Monkey_WalkRight", monkeySpriteSheet, 0, 8, 16, 16, 75, true, as);
        walkLeft = CreateAnimation("Monkey_WalkLeft", monkeySpriteSheet, 9, 17, 16, 16, 75, true, as);
        idle = CreateAnimation("Monkey_Idle", monkeySpriteSheet, 18, 23, 16, 16, 75, true, as);

        //...pack the animations in a NULL terminated array
        sot_sprite_t **animations = malloc(sizeof(sot_sprite_t*) * 4);
        animations[0] = idle;
        animations[1] = walkLeft;
        animations[2] = walkRight;
        animations[3] = NULL;

        // Create the player actor and add it to the scene.
        // ...get the player start position from the marker in the tiled-map
        cute_tiled_object_t *currentObject = currentScene->sot_tilemap->tilemap->layers[1].objects;
        char *playerStart = "player_start";
        vec2 startPosition = {0,0};
        while (currentObject != NULL) {
            if (strcmp(currentObject->name.ptr, playerStart) == 0) {
                vec2 currentPosition = { currentObject->x, currentObject->y };
                glm_vec2_copy(currentPosition, startPosition);
            }
            currentObject = currentObject->next;
        }
        
        sot_actor_t *player = CreateActor(as, "Player", startPosition, animations, C2_TYPE_CIRCLE);
        currentScene->player = player;

    }
    
    return currentScene;
}

void UpdateScene(AppState *as, SOT_Scene * scene, float deltaTime) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status

    //UpdateCamera(&scene->worldCamera, (vec3) {0,0,-1}, deltaTime, 1);
    // UpdateActor(as, scene->player, deltaTime);

    return;
}

void SOT_GPU_RenderScene(SOT_Scene *scene, SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi)
{
    SOT_GPU_RenderTilemap(scene->sot_tilemap, gpu, rpi, scene->worldCamera.pvMatrix);

    
    
    // ------------------------------------------------- Render Actors Section ----------------------------------------------------------//
    // ------------------------------------------------- Render UI Section --------------------------------------------------------------//    
}

void DestroyScene(SOT_Scene * scene) {
    DestroyActor(scene->player);
    DestroyTilemap(scene->sot_tilemap);
    free(scene);
}