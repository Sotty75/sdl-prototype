#ifndef ACTOR_H
#define ACTOR_H

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "cglm.h"
#include "cute_c2.h"
#include "appstate.h"
#include "sot_physics.h"
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
    SOT_Transform transform;
    SOT_Physics physics;
    SOT_Animation animations[256];
    int currentAnimation;
    // ----------------------------- collision data
    sot_collider_t collider;
    c2Manifold collisionInfo[50];
} SOT_Actor;

/**
 * @brief Creates a new actor instance with the specified properties.
 * 
 * Allocates memory for a new SOT_Actor, initializes its position, velocity, and 
 * loads the associated animations from the specified file.
 * 
 * @param appState Pointer to the global application state.
 * @param name The name of the actor.
 * @param pos The initial position (x, y) of the actor.
 * @param animationFile Filename of the JSON file containing animation definitions.
 * @return Pointer to the newly created SOT_Actor, or NULL on failure.
 */
SOT_Actor *SOT_CreateActor(AppState *appState, char *name, vec2 pos, char *animationFile);

/**
 * @brief Binds loaded animation information to an actor.
 * 
 * Processes the raw animation info loaded from a file and sets up the actor's 
 * internal animation structures.
 * 
 * @param actor Pointer to the actor to bind animations to.
 * @param animationInfo Pointer to the loaded animation information structure.
 */
void SOT_ActorBindAnimations(SOT_Actor *actor, SOT_AnimationInfo *animationInfo);

/**
 * @brief Sets the position of the actor.
 * 
 * @param actor Pointer to the actor.
 * @param pos The new position vector (x, y).
 */
void SetPosition(SOT_Actor *actor, vec2 pos);

/**
 * @brief Sets the velocity of the actor.
 * 
 * @param actor Pointer to the actor.
 * @param vel The new velocity vector (vx, vy).
 */
void SetVelocity(SOT_Actor *actor, vec2 vel);

/**
 * @brief Assigns a collider to the actor.
 * 
 * @param actor Pointer to the actor.
 * @param collider The collider structure to assign.
 */
void SetCollider(SOT_Actor *actor, sot_collider_t collider);

/**
 * @brief Updates the position of the actor's collider by applying a delta offset.
 * 
 * This function modifies the internal shape data of the collider (e.g., AABB min/max or Circle center)
 * by adding the provided delta vector to the current coordinates.
 * 
 * @param actor Pointer to the actor whose collider needs to be updated.
 * @param deltaPos The change in position (x, y) to apply to the collider.
 */
void UpdateCollider(SOT_Actor *actor, vec2 deltaPos);


void MoveActor(SOT_Actor *actor, SDL_Event *event);
void Hit(const AppState *as, SOT_Actor *actor);
void UpdateActor(const AppState *as, SOT_Actor *actor, float deltaTime);
void SetRenderPosition(SOT_Actor *actor);
void RenderActor(const AppState *as, SOT_Actor *actor);
void DestroyActor(SOT_Actor *actor);

#endif // ACTOR_H
