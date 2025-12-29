#ifndef ACTOR_H
#define ACTOR_H

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "cglm.h"
#include "cute_c2.h"
#include "appstate.h"
#include "sot_sprite.h"
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

typedef struct sot_actor_t {
    char *name;
    bool applyGravity;
    vec2 position;
    vec2 velocity;
    SDL_FRect renderRect;
    sot_sprite_t **pSprites;
    sot_sprite_t *pCurrentSprite;
    Uint64 lastStep;
    Direction direction;
    sot_collider_t collider;
    c2Manifold collisionInfo[50];
} sot_actor_t;

sot_actor_t *CreateActor(AppState *appState, char *name, vec2 pos, sot_sprite_t **anims, C2_TYPE colliderType); 
void SetPosition(sot_actor_t *actor, vec2 pos);
void SetVelocity(sot_actor_t *actor, vec2 vel);
void SetCollider(sot_actor_t *actor, sot_collider_t collider);
void UpdateCollider(sot_actor_t *actor);
void MoveActor(sot_actor_t *actor, SDL_Event *event);
void Hit(const AppState *as, sot_actor_t *actor);
void UpdateActor(const AppState *as, sot_actor_t *actor, float deltaTime);
void SetRenderPosition(sot_actor_t *actor);
void RenderActor(const AppState *as, sot_actor_t *actor);
void DestroyActor(sot_actor_t *actor);

#endif // ACTOR_H
