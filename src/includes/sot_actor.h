#ifndef ACTOR_H
#define ACTOR_H

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "cglm.h"
#include "cute_c2.h"
#include "appstate.h"
#include "sot_animation.h"
#include "sot_collider.h"
#include "sot_math_interop.h"



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

typedef struct SOT_Actor {
    int actorID;                // --> Used as an index in the batch sprites rendering array. At this specific index, we will store the spritinfo for the current frame.
    char *actorName;
    char *atlasName;
    bool applyGravity;
    vec2 position;
    vec2 velocity;
    SOT_Animation **animations;
    SOT_Animation *activeAnimation;
    Uint64 lastStep;
    Direction direction;
    sot_collider_t collider;
    c2Manifold collisionInfo[50];
} SOT_Actor;

SOT_Actor *SOT_CreateActor(AppState *appState, char *name, vec2 pos, char *animationFile);
void SetPosition(SOT_Actor *actor, vec2 pos);
void SetVelocity(SOT_Actor *actor, vec2 vel);
void SetCollider(SOT_Actor *actor, sot_collider_t collider);
void UpdateCollider(SOT_Actor *actor);
void MoveActor(SOT_Actor *actor, SDL_Event *event);
void Hit(const AppState *as, SOT_Actor *actor);
void UpdateActor(const AppState *as, SOT_Actor *actor, float deltaTime);
void SetRenderPosition(SOT_Actor *actor);
void RenderActor(const AppState *as, SOT_Actor *actor);
void DestroyActor(SOT_Actor *actor);

#endif // ACTOR_H
