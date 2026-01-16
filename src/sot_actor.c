#include "sot_actor.h"

SOT_Actor *CreateActor(AppState *appState, char *name, vec2 pos, char *animationsFile) 
{
    SOT_Actor *actor = malloc(sizeof(SOT_Actor));
    if (actor == NULL) return NULL;

    actor->actorName = name;
    actor->applyGravity = true;

    // set actor position and velocity
    actor->position[0]=pos[0];
    actor->position[1]=pos[1];
    actor->velocity[0]=10;
    actor->velocity[1]=0;
   
    // Bind animations to actor
    SOT_AnimationInfo *animationInfo = SOT_LoadAnimations(animationsFile);

    // TODO: Modify the sprite editor tool to include also the collider type
    // TODO: Initialize the animation data and uplaod the data on the GPU Memory
    // TODO: Destory the animation info object

    return actor;
}

void SOT_BindActorAnimations(SOT_Actor *actor, SOT_AnimationInfo *animationInfo)
 {
    
   /*  // Bind the animations to the actor
    actor->animations = anims;
    actor->lastStep = SDL_GetTicks();
    actor->activeAnimation = actor->animations[0];

    // Initialize the collider

    // Collider Initialization
    sot_collider_t collider = {0};

    switch (colliderType) {
        case C2_TYPE_CIRCLE:
            collider.type = C2_TYPE_CIRCLE;
            collider.shape.circle.p = *vec2_to_c2v(&actor->position);
            collider.shape.circle.r = actor->animations[0]->info->frameSize[0]/2;
            break;
        case C2_TYPE_AABB:
            collider.type = C2_TYPE_AABB;
            collider.shape.AABB.min.x = -actor->animations[0]->info->frameSize[0]/2;
            collider.shape.AABB.max.x = actor->animations[0]->info->frameSize[0]/2;
            collider.shape.AABB.min.y = -actor->animations[0]->info->frameSize[0]/2;
            collider.shape.AABB.max.y = actor->animations[0]->info->frameSize[0]/2;
            break;
        case C2_TYPE_NONE:
        case C2_TYPE_CAPSULE:
        case C2_TYPE_POLY:
        default:
        collider.type = C2_TYPE_NONE;
            break;
    }
    
    actor->collider = collider;

    AppendCollider(appState->pDynamicColliders, &(actor->collider)); */
}

void SetPosition(SOT_Actor *actor, vec2 pos) { 
    glm_vec2_copy(pos, actor->position);
    return; 
}

void SetVelocity(SOT_Actor *actor, vec2 pos) { 
    glm_vec2_copy(pos, actor->velocity);
    return; 
}

void SetCollider(SOT_Actor *actor, sot_collider_t collider) {
    actor->collider = collider;
    return;
}

/*
 *   Updates the collider poition / geometry to follow the sprite
 *   current position while moving on the screen.
 */
void UpdateCollider(SOT_Actor *actor) {
    sot_collider_t *collider = &(actor->collider);

    switch (collider->type) {
        case C2_TYPE_CIRCLE:
            collider->shape.circle.p = *vec2_to_c2v(&actor->position);
            collider->shape.circle.r = actor->animations[0]->info->frameSize[0]/2;
            break;
        case C2_TYPE_AABB:
            collider->type = C2_TYPE_AABB;
            collider->shape.AABB.min.x = -actor->animations[0]->info->frameSize[0]/2;
            collider->shape.AABB.max.x = actor->animations[0]->info->frameSize[0]/2;
            collider->shape.AABB.min.y = -actor->animations[0]->info->frameSize[0]/2;
            collider->shape.AABB.max.y = actor->animations[0]->info->frameSize[0]/2;
            break;
        case C2_TYPE_NONE:
        case C2_TYPE_CAPSULE:
        case C2_TYPE_POLY:
        default:
            collider->type = C2_TYPE_NONE;
            break;
    }

}



// Updates the renderRect position based on the the current actor position.
// Position is applied to the center of the sprite.
void SetRenderPosition(SOT_Actor *actor) 
{
    int deltaX = actor->activeAnimation->info->frameSize[0]/2;
    int deltaY = actor->activeAnimation->info->frameSize[1]/2;

    actor->renderRect = (SDL_FRect) {
        .x = actor->position[0]-deltaX,
        .y = actor->position[1]-deltaY,
        .w = actor->activeAnimation->info->frameSize[0],
        .h = actor->activeAnimation->info->frameSize[1]
    };
}

void RenderActor(const AppState *as, SOT_Actor *actor) {

    SOT_Animation *animation = actor->activeAnimation;

    // Get the current time different from last step
    // if higher than the animation time step
    // update the texture with the new frame surface.
    const Uint64 now = SDL_GetTicks();

    // This while will executed once as soon as now reached
    // the value of the stepRateMillis, it will exit at the first execution
    // as we immediately reset last_step to now
    while ((now - actor->lastStep) >= animation->stepRateMillis) 
    {
        // navigate to the next frame in the linked list of frames
        // animation->currentFrame = animation->currentFrame->next;
        actor->lastStep = now;            
    }

    // SetRenderPosition(actor);
    // SDL_RenderTexture(appState->gpu->renderer, animation->atlas, animation->currentFrame->sprite, &(actor->renderRect));
    DrawCollidersDebugInfo(as->gpu, actor->collider);

    return;
}


void MoveActor(SOT_Actor *actor, SDL_Event *event) 
{
    if (actor == NULL) return;

    switch (event->type)
    {
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            if (event->gaxis.axis == 0) {
                if (event->gaxis.value > GAMEPAD_DEADZONE && actor->direction != MOVE_RIGHT) { actor->direction = MOVE_RIGHT; }
                else if (event->gaxis.value < -GAMEPAD_DEADZONE && actor->direction != MOVE_LEFT) { actor->direction = MOVE_LEFT; }
                else if (event->gaxis.value >= -GAMEPAD_DEADZONE && event->gaxis.value <= GAMEPAD_DEADZONE) { actor->direction = IDLE; }
            }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            actor->direction = JUMP;
            break;
        default:
            break;
    }

    
    return; 
}

void Hit(const AppState *as, SOT_Actor *actor) {
    
    memset(actor->collisionInfo, 0, sizeof(actor->collisionInfo));
    int hit = false;
    
    C2_TYPE actorType;
    void *actorShape;

    // rectrieve actorShape;
    switch (actor->collider.type) {
        case C2_TYPE_CIRCLE:
            actorType = C2_TYPE_CIRCLE;
            actorShape = &(actor->collider.shape.circle);
            break;
        case C2_TYPE_AABB:
            actorType = C2_TYPE_AABB;
            actorShape = &(actor->collider.shape.AABB);
            break;
        case C2_TYPE_NONE:
        case C2_TYPE_CAPSULE:
        case C2_TYPE_POLY:
        default:
            actorType = C2_TYPE_NONE;
            actorShape = NULL;
            break;
    }

    if (actorShape == NULL) return;

    int hitsCount = 0;
    sot_collider_node_t *staticCollider = as->pStaticColliders;
    // for each static collider check the collision between the actor and the collider
    while (staticCollider != NULL) {
        // get static collider info

        C2_TYPE colliderType;
        void *colliderShape;

        // rectrieve actorShape;
        switch (staticCollider->collider->type) {
            case C2_TYPE_CIRCLE:
                colliderType = C2_TYPE_CIRCLE;
                colliderShape = &(staticCollider->collider->shape.circle);
                break;
            case C2_TYPE_AABB:
                colliderType = C2_TYPE_AABB;
                colliderShape = &(staticCollider->collider->shape.AABB);
                break;
            case C2_TYPE_NONE:
            case C2_TYPE_CAPSULE:
            case C2_TYPE_POLY:
            default:
                colliderType = C2_TYPE_NONE;
                colliderShape = NULL;
                break;
        }

        // get collision info
        c2Manifold  collisionInfo = {0};  
        c2Collide(actorShape, NULL, actorType, colliderShape, NULL, colliderType, &collisionInfo);
        if (collisionInfo.count > 0) {
            actor->collisionInfo[hitsCount] = collisionInfo;
            hitsCount++;
        }
        
        staticCollider = staticCollider->next;
    }

    return;
}

void UpdateActor(const AppState *as, SOT_Actor *actor, float deltaTime) {

    if (actor == NULL)
        return;

    if (actor->applyGravity)
        actor->direction = FALL;
    
    // to evaluate the collision when we move we need to understand if in the direction of the movement we can move or not
    // so we need to be aware of any collision that is in place. Basically, if in the previous frame there was a collision and we identified it as
    // a collision stopping any movement in a given direction, we need to take it in account when evaluating the new position of the 
    // actor. 
    // The logic can be then implemented by evaluationg a collision, storing the outcome in some structure linked to the actor wher
    // maybe we can store the constraint, maybe as enums (COOLLISION_RIGHT, COLLISION_LEFT...). THen we can use it in the 
    // switch updating the position to decide if we want or not to apply the collision. 
    // Thinking about it, maybe rather than an enum we may have an actual vector that describes the constraint force applied to the movement of the player.
    // Based on the strenght of this force (or maybe just the velocity) we cna adjust the new position in different directions.
    // That means that even when moving left or right, in case of a slope, we will adjust the vertical position.

    switch (actor->direction)
    {
        case MOVE_RIGHT:
            if (actor->activeAnimation != actor->animations[2]) actor->activeAnimation = actor->animations[2];
            actor->position[0] += actor->velocity[0] * deltaTime;
            break;
        case MOVE_LEFT:
            if (actor->activeAnimation != actor->animations[1]) actor->activeAnimation = actor->animations[1];
            actor->position[0] -= actor->velocity[0] * deltaTime;
            break;
        case FALL:
            actor->position[1] += 2 * deltaTime;
            break;
        case IDLE:
            if (actor->activeAnimation != actor->animations[0]) actor->activeAnimation = actor->animations[0];
        default:
            break;
    }

    // Update the position of the attached collider.
    UpdateCollider(actor);
    Hit(as, actor);

    return;
}

void DestroyActor(SOT_Actor *actor) { 
    if (actor == NULL) return;
    
    for (int i = 0; actor->animations[i] != NULL; i++) {
        DestroyAnimation(actor->animations[i]);
    }

    free(actor);
    return; 
}