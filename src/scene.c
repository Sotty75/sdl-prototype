#include "sot_scene.h"
#include "sot_sprites.h"
#include "sot_actor.h"



Scene *CreateScene(AppState *appState) {
    // Create all the actors
    // Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
    // player, later we will use a file as an input (JSON, XML....)

    Scene *currentScene = malloc(sizeof(Scene));
    if (currentScene == NULL) return NULL;

    // set the scene ID
    currentScene->id = 1;

    //... load the spritesheet inside of the texture
    SDL_Surface *monkeySpriteSheet = NULL;
    Sprite *walkRight = NULL;
    Sprite *walkLeft = NULL;
    Sprite *idle = NULL;
    GetSurfaceFromImage(&monkeySpriteSheet, "monkey-sheet.png");
    walkRight = CreateSprite("Monkey", monkeySpriteSheet, 0, 8, 16, 16, 75, true, appState->renderer);
    walkLeft = CreateSprite("Monkey", monkeySpriteSheet, 9, 17, 16, 16, 75, true, appState->renderer);
    idle = CreateSprite("Monkey", monkeySpriteSheet, 18, 23, 16, 16, 75, true, appState->renderer);

    //...pack the animations in a NULL terminated array
    Sprite **animations = malloc(sizeof(Sprite*) * 4);
    animations[0] = walkRight;
    animations[1] = walkLeft;
    animations[2] = idle;
    animations[3] = NULL;

    //...create the player actor and add it to the scene
    vec2 pos = {0,0};
    vec2 vel = {0,0};   
    Actor *player = CreateActor("Player", pos, vel, animations);
    currentScene->player = player;

    return currentScene;
}


void UpdateScene(Scene * scene) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status

    // update camera
}


void RenderScene(AppState *appState, Scene * scene) {

    // for each actor in the scene, render it on the screen.
    RenderActor(scene->player, appState);    
}


void DestroyScene(Scene * scene) {
    DestroyActor(scene->player);
    free(scene);
}