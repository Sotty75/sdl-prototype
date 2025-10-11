#ifndef ACTOR_H
#define ACTOR_H

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "appstate.h"
#include "cglm.h"
#include "sot_animation.h"

/*
    +-------+
    |       |
    |       |
    +-------+
   (x,y)
*/

#define GAMEPAD_DEADZONE 8000


typedef enum {
    IDLE,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN,
    FALL,
    JUMP
} Direction;

typedef struct Actor {
    char *name;
    bool applyGravity;
    vec2 pos;
    vec2 vel;
    SDL_FRect position;
    Animation **anims;
    Animation *currentAnim;
    Uint64 last_step;
    Direction direction;
} Actor;

Actor *CreateActor(char *name, vec2 pos, Animation **anims); 
void SetPosition(Actor *actor, vec2 pos);
void SetVelocity(Actor *actor, vec2 vel);
void MoveActor(Actor *actor, SDL_Event *event);
void UpdateActor(Actor *actor, float deltaTime);
void RenderActor(Actor *actor, AppState *appState);
void DestroyActor(Actor *actor);

#endif // ACTOR_H
