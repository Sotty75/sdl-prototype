#include <stdlib.h>
#include "sot_actor.h"
#include "appstate.h"



Actor *CreateActor(char *name, vec2 pos, Animation **anims) 
{
    Actor *actor = malloc(sizeof(Actor));
    if (actor == NULL) return NULL;

    actor->name = name;
    actor->applyGravity = true;

    // set actor position and velocity
    actor->pos[0]=pos[0];
    actor->pos[1]=pos[1];
    actor->vel[0]=10;
    actor->vel[1]=0;

    actor->anims = anims;
    actor->last_step = SDL_GetTicks();
    actor->currentAnim = actor->anims[0];
    actor->position = (SDL_FRect){
        .x = pos[0],
        .y = pos[1], 
        .w = 16,
        .h = 16};

    return actor;
}


void SetPosition(Actor *actor, vec2 pos) { 
    glm_vec2_copy(pos, actor->pos);
    return; 
}

void SetVelocity(Actor *actor, vec2 pos) { 
    glm_vec2_copy(pos, actor->vel);
    return; 
}

void RenderActor(Actor *actor, AppState *appState) {

    Animation *animation = actor->currentAnim;

    // Get the current time different from last step
    // if higher than the animation time step
    // update the texture with the new frame surface.
    const Uint64 now = SDL_GetTicks();

    // This while will executed once as soon as now reached
    // the value of the stepRateMillis, it will exit at the first execution
    // as we immediately reset last_step to now
    while ((now - actor->last_step) >= animation->stepRateMillis) 
    {
        // navigate to the next frame in the linked list of frames
        animation->currentFrame = animation->currentFrame->next;
        actor->last_step = now;            
    }

    SDL_RenderTexture(appState->renderer, animation->atlas, animation->currentFrame->sprite, &(actor->position));

    return;
}


void MoveActor(Actor *actor, SDL_Event *event) 
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

void UpdateActor(Actor *actor, float deltaTime) {

    if (actor == NULL)
        return;

    if (actor->applyGravity)
        actor->direction = FALL;
    
    switch (actor->direction)
    {
        case MOVE_RIGHT:
            if (actor->currentAnim != actor->anims[2]) actor->currentAnim = actor->anims[2];
            actor->position.x += actor->vel[0] * deltaTime;
            break;
        case MOVE_LEFT:
            if (actor->currentAnim != actor->anims[1]) actor->currentAnim = actor->anims[1];
            actor->position.x -= actor->vel[0] * deltaTime;
            break;
        case FALL:
            actor->position.y += 80 * deltaTime;
            break;
        case IDLE:
            if (actor->currentAnim != actor->anims[0]) actor->currentAnim = actor->anims[0];
        default:
            break;
    }
}

void DestroyActor(Actor *actor) { 
    if (actor == NULL) return;
    
    for (int i = 0; actor->anims[i] != NULL; i++) {
        DestroySprite(actor->anims[i]);
    }

    free(actor);
    return; 
}