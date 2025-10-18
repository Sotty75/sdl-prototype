#define CUTE_TILED_IMPLEMENTATION

#include "sot_scene.h"




Scene *CreateScene(AppState *as) {
    // Create all the actors
    // Put the actors in the scene, for the time being we will hardcode the create scene logi to my test
    // player, later we will use a file as an input (JSON, XML....)

    Scene *currentScene = malloc(sizeof(Scene));
    if (currentScene == NULL) return NULL;

    // set the scene ID
    currentScene->id = 1;

    // ...create the tilemap and include it into the scene
    char *assetName = "level.json";
    currentScene->sot_tilemap = CreateTilemap(assetName, as);
    if (currentScene->sot_tilemap == NULL) return NULL;

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
    
    sot_actor_t *player = CreateActor(as, "Player", startPosition, animations, C2_TYPE_CIRCLE);
    currentScene->player = player;

    return currentScene;
}


void UpdateScene(AppState *as, Scene * scene, float deltaTime) {

    // receives the input from the player as an appstate
    // recalculate actors position (collision check)
    // update game status

    // update camera
    UpdateActor(as, scene->player, deltaTime);
    return;
}


void RenderScene(AppState *as, Scene * scene) {

    // render the map
    RenderTilemap(scene->sot_tilemap, as);

    // for each actor in the scene, render it on the screen.
    RenderActor(as, scene->player);    
}

void DestroyScene(Scene * scene) {
    DestroyActor(scene->player);
    DestroyTilemap(scene->sot_tilemap);
    free(scene);
}