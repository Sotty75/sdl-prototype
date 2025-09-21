#ifndef ACTOR_H
#define ACTOR_H

#include <stdlib.h>
#include "cglm.h"
#include "sot_sprites.h"
#include "appstate.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

/*
    +-------+
    |       |
    |       |
    +-------+
   (x,y)
*/

typedef enum {
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN,
    JUMP
} Directions;


typedef struct {
    char *name;
    vec2 pos;
    vec2 vel;
    SDL_FRect postion;
    Sprite **anims;
    Sprite *currentAnim;
    Uint64 last_step;
} Actor;

Actor *CreateActor(char *name, vec2 pos, vec2 vel, Sprite **anims); 
void SetPosition(Actor *actor, vec2 pos);
void SetVelocity(Actor *actor, vec2 vel);
void Move(Directions direction);
void RenderActor(Actor *actor, AppState *appState);
void DestroyActor(Actor *actor);

#endif // ACTOR_H
