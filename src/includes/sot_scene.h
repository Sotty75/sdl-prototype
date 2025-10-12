#ifndef SCENE_H_
#define SCENE_H_

#include <stdlib.h>
#include "appstate.h"
#include "sot_actor.h"
#include "sot_tilemap.h"
#include "cute_tiled.h"


typedef struct sot_scene_t {

    int id;
    sot_actor_t *player;
    sot_tilemap_t *sot_tilemap;
} sot_scene_t;


sot_scene_t* CreateScene(AppState *appState);


/*****************************************************
 * 
 *   Iterates through all the entities of the scene to update the status based on t
 *   the current game state / player input
 * 
 *****************************************************/
void UpdateScene(sot_scene_t * scene, float deltaTime);

/*****************************************************
 * 
 *   Renders the scene on the screen
 *    
 *****************************************************/
void RenderScene(AppState *appState, sot_scene_t * scene);
void DestroyScene(sot_scene_t * scene);

#endif
