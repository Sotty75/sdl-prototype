#include "sot_actor.h"

SOT_Actor *SOT_CreateActor(AppState *appState, char *name, vec2 pos, char *animationsFile) 
{
    SOT_Actor *actor = malloc(sizeof(SOT_Actor));
    
    if (actor == NULL) 
        return NULL;
    else 
        SDL_memset(actor, 0, sizeof(actor));

    actor->actorName = name;
    actor->physics.body = SOT_PHB_KINEMATIC;

    // set actor position and velocity
    actor->transform.position[0] = pos[0];
    actor->transform.position[1] = pos[1];
    actor->physics.v_magnitude = 10;
    glm_vec2_copy((vec2){1.0f, 0.0f}, actor->physics.v_direction);
   
    SOT_AnimationInfo *animationInfo = SOT_LoadAnimations(animationsFile); 
    SOT_ActorBindAnimations(actor, animationInfo);
    
    // TODO: Remove the Direction field from the actor, and make it a property of the animation (ACTION)
    //  typedef enum {
    //     IDLE,
    //     MOVE_RIGHT,
    //     MOVE_LEFT,
    //     MOVE_UP,
    //     MOVE_DOWN,
    //     FALL,
    //     JUMP
    // } Direction;
    // this will require to modify again the sprite editor and the animation json file.
    // instead of using an enum i think it is better to use a string with some constants already predefined so we
    // can create custom actions later. For hte moment i will remove it as i can rely on the animation name.
    // TODO: Initialize the sprite rendering pipeline (probably in a different file).
    // TODO: Upload the atlas to the gpu. 

    SDL_free(animationInfo);        
    return actor;
}

void SOT_ActorBindAnimations(SOT_Actor *actor, SOT_AnimationInfo *animationInfo)
 {
    //...bind the animation data to for each animation to the actor
    for (int i = 0; i < animationInfo->count; ++i) 
    {
        SOT_Animation *anim = &(actor->animations[i]);
        
        anim->id = i;
        anim->atlasName = animationInfo->atlasName;
        SDL_memcpy(&(anim->data), &(animationInfo->data[i]), sizeof(SOT_AnimationData));
        anim->gpuInfo = NULL;
        anim->step_ms = 75;

        actor->currentAnimation = 0;
    }

    
    //... load the spritesheet inside of the texture
        /* SDL_Surface *monkeySpriteSheet = NULL;
        SOT_Animation *walkRight = NULL;
        SOT_Animation *walkLeft = NULL;
        SOT_Animation *idle = NULL;
        walkRight = CreateAnimation("Monkey_WalkRight", monkeySpriteSheet, 0, 8, 16, 16, 75, true, as);
        walkLeft = CreateAnimation("Monkey_WalkLeft", monkeySpriteSheet, 9, 17, 16, 16, 75, true, as);
        idle = CreateAnimation("Monkey_Idle", monkeySpriteSheet, 18, 23, 16, 16, 75, true, as);

        //...pack the animations in a NULL terminated array
        SOT_Animation **animations = malloc(sizeof(SOT_Animation*) * 4);
        animations[0] = idle;
        animations[1] = walkLeft;
        animations[2] = walkRight;
        animations[3] = NULL; */


    /* Bind the animations to the actor
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
    glm_vec2_copy(pos, actor->transform.position);
    return; 
}

void SetVelocity(SOT_Actor *actor, vec2 vel) { 
    actor->physics.v_magnitude = glm_vec2_norm(vel);
    if (actor->physics.v_magnitude > GLM_FLT_EPSILON) {
        glm_vec2_normalize_to(vel, actor->physics.v_direction);
    }
    return; 
}

void SetCollider(SOT_Actor *actor, sot_collider_t collider) {
    actor->collider = collider;
    return;
}

void UpdateCollider(SOT_Actor *actor, vec2 deltaPos) {
    sot_collider_t *collider = &(actor->collider);

    switch (collider->type) {
        case C2_TYPE_CIRCLE:
            collider->shape.circle.p.x += deltaPos[0];
            collider->shape.circle.p.y += deltaPos[1];
            break;
        case C2_TYPE_AABB:
            collider->type = C2_TYPE_AABB;
            collider->shape.AABB.min.x += deltaPos[0];
            collider->shape.AABB.min.y += deltaPos[1];
            collider->shape.AABB.max.x += deltaPos[0];
            collider->shape.AABB.max.y += deltaPos[1];
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
    // int deltaX = actor->activeAnimation->info->frameSize[0]/2;
    // int deltaY = actor->activeAnimation->info->frameSize[1]/2;

    // actor->renderRect = (SDL_FRect) {
    //     .x = actor->position[0]-deltaX,
    //     .y = actor->position[1]-deltaY,
    //     .w = actor->activeAnimation->info->frameSize[0],
    //     .h = actor->activeAnimation->info->frameSize[1]
    // };
}

void RenderActor(const AppState *as, SOT_Actor *actor) {

    SOT_Animation *animation = &(actor->animations[actor->currentAnimation]);
    SOT_AnimationData *frames = &(animation->data);

    // TODO: Move this logic to a different function such as, for example, UpdateAnimation which can be invoked in the UpdateActor,
    // The render operation will be done once th status of the sprite has been updated.

    // Get the current time different from last step
    // if higher than the animation time step
    // update the texture with the new frame surface.
    const Uint64 now = SDL_GetTicks();

    // This while will executed once as soon as now reached
    // the value of the stepRateMillis, it will exit at the first execution
    // as we immediately reset last_step to now
    if ((now - as->last_step) >= animation->step_ms) 
    {
        // navigate to the next frame in the linked list of frames
        frames->current = (frames->current == frames->count ) ? 0 : frames->current + 1;
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
                if (event->gaxis.value > GAMEPAD_DEADZONE && event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
                        glm_vec2_copy((vec2){1.0f, 0.0f}, actor->physics.v_direction);
                    }
                else if (event->gaxis.value < -GAMEPAD_DEADZONE && event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
                        glm_vec2_copy((vec2){-1.0f, 0.0f}, actor->physics.v_direction);
                    }
                else if (event->gaxis.value >= -GAMEPAD_DEADZONE && event->gaxis.value <= GAMEPAD_DEADZONE) {
                        glm_vec2_copy((vec2){0.0f, 0.0f}, actor->physics.v_direction);
                    }
            }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            glm_vec2_copy((vec2){0.0f, 1.0f}, actor->physics.v_direction);
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
    vec2 velocity;
    glm_vec2_scale(actor->physics.v_direction, actor->physics.v_magnitude, velocity);
    vec2 deltaPos = {0, 0};

    // Maybe it is time to switch to a STATES MACHINE??? 
    /*
    switch (actor->direction)
    {
        case MOVE_RIGHT:
            if (actor->activeAnimation != actor->animations[2]) actor->activeAnimation = actor->animations[2];
            actor->position[0] += actor->velocity[0] * deltaTime;
            if (actor->activeAnimation != &actor->animations[2]) 
                actor->activeAnimation = &actor->animations[2];
            
            deltaPos[0] = velocity[0] * deltaTime;
            actor->transform.position[0] += deltaPos[0];
            break;
        case MOVE_LEFT:
            if (actor->activeAnimation != actor->animations[1]) actor->activeAnimation = actor->animations[1];
            actor->position[0] -= actor->velocity[0] * deltaTime;
            if (actor->activeAnimation != &actor->animations[1]) 
                actor->activeAnimation = &actor->animations[1];
            
            // Assuming velocity is a magnitude-based vector, we invert for left if direction is just a speed scalar
            // However, if v_direction is (1,0), we need to negate.
            deltaPos[0] = -velocity[0] * deltaTime;
            actor->transform.position[0] += deltaPos[0];
            break;
        case FALL:
            actor->position[1] += 2 * deltaTime;
            deltaPos[1] = 2 * deltaTime; // Simple gravity simulation
            actor->transform.position[1] += deltaPos[1];
            break;
        case IDLE:
            if (actor->activeAnimation != actor->animations[0]) actor->activeAnimation = actor->animations[0];
            if (actor->activeAnimation != &actor->animations[0]) 
                actor->activeAnimation = &actor->animations[0];
        default:
            break;
    }

    // Update the position of the attached collider.
    UpdateCollider(actor, deltaPos);
    Hit(as, actor);
    */
    return;
}

void DestroyActor(SOT_Actor *actor) { 
    if (actor == NULL) return;
    /*
    for (int i = 0; actor->animations[i] != NULL; i++) {
        DestroyAnimation(actor->animations[i]);
    }
    */
    free(actor);
    return; 
}