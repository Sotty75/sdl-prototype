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

    char *tileMapPath = NULL;
    char *assetName = "level.json";

    //...load the tilemap from a json file stored in the assets folder.
    SDL_asprintf(&tileMapPath, "%sassets\\%s", SDL_GetBasePath(), assetName);
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(tileMapPath, NULL);
    int width = map->width;
    int height = map->height;

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

    //...create the player actor and add it to the scene
    vec2 pos = {0,0};
    vec2 vel = {40,0};   
    Actor *player = CreateActor("Player", pos, vel, animations);
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

    // for each actor in the scene, render it on the screen.
    RenderActor(scene->player, appState);    
}

void DestroyScene(Scene * scene) {
    DestroyActor(scene->player);
    free(scene);
}