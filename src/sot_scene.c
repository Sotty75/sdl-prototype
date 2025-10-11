#define CUTE_TILED_IMPLEMENTATION

#include "sot_scene.h"
#include "sot_animation.h"
#include "sot_actor.h"
#include "cute_tiled.h"



Scene *CreateScene(AppState *appState) {
    // Create all the actors
    // Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
    // player, later we will use a file as an input (JSON, XML....)

    Scene *currentScene = malloc(sizeof(Scene));
    if (currentScene == NULL) return NULL;

    // set the scene ID
    currentScene->id = 1;

    // ...create the tilemap and include it into the scene
    char *assetName = "level.json";
    currentScene->sot_tilemap = CreateTilemap(assetName, appState);
    if (currentScene->sot_tilemap == NULL) return NULL;

    //... load the spritesheet inside of the texture
    SDL_Surface *monkeySpriteSheet = NULL;
    Animation *walkRight = NULL;
    Animation *walkLeft = NULL;
    Animation *idle = NULL;
    walkRight = CreateAnimation("Monkey_WalkRight", monkeySpriteSheet, 0, 8, 32, 32, 75, true, appState);
    walkLeft = CreateAnimation("Monkey_WalkLeft", monkeySpriteSheet, 9, 17, 32, 32, 75, true, appState);
    idle = CreateAnimation("Monkey_Idle", monkeySpriteSheet, 18, 23, 32, 32, 75, true, appState);

    //...pack the animations in a NULL terminated array
    Animation **animations = malloc(sizeof(Animation*) * 4);
    animations[0] = idle;
    animations[1] = walkLeft;
    animations[2] = walkRight;
    animations[3] = NULL;

    // Create the player actor and add it to the scene.
    // ...get the player start position from the marker in the tiled-map
    cute_tiled_layer_t *mapLayer = &currentScene->sot_tilemap->tilemap->layers[0];
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
    
    Actor *player = CreateActor("Player", startPosition, animations);
    currentScene->player = player;

    return currentScene;
}


void UpdateScene(Scene * scene, float deltaTime) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status

    // update camera
    UpdateActor(scene->player, deltaTime);
    return;
}


void RenderScene(AppState *appState, Scene * scene) {

    // render the map
    RenderTilemap(scene->sot_tilemap, appState);

    // for each actor in the scene, render it on the screen.
    RenderActor(scene->player, appState);    
}

void DestroyScene(Scene * scene) {
    DestroyActor(scene->player);
    DestroyTilemap(scene->sot_tilemap);
    free(scene);
}