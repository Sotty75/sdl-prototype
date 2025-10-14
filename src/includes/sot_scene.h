#ifndef SCENE_H_
#define SCENE_H_

#include <stdlib.h>
#include "appstate.h"
#include "sot_actor.h"
#include "sot_sprite.h"
#include "sot_tilemap.h"
#include "cute_tiled.h"



typedef struct Scene {
    int id;
    sot_actor_t *player;
    sot_tilemap_t *sot_tilemap;
} Scene;


Scene* CreateScene(AppState *appState);


/*****************************************************
 * 
 *   Iterates through all the entities of the scene to update the status based on t
 *   the current game state / player input
 * 
 *****************************************************/
void UpdateScene(Scene * scene, float deltaTime);

/*****************************************************
 * 
 *   Renders the scene on the screen
 *    
 *****************************************************/
void RenderScene(AppState *appState, Scene * scene);
void DestroyScene(Scene * scene);

#endif
