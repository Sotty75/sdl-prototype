#include <stdlib.h>
#include "sot_actor.h"
#include "appstate.h"



sot_actor_t *CreateActor(char *name, vec2 pos, sot_sprite_t **anims) 
{
    sot_actor_t *actor = malloc(sizeof(sot_actor_t));
    if (actor == NULL) return NULL;

    actor->name = name;
    actor->applyGravity = true;

    // set actor position and velocity
    actor->position[0]=pos[0];
    actor->position[1]=pos[1];
    actor->velocity[0]=10;
    actor->velocity[1]=0;

    actor->sprites = anims;
    actor->last_step = SDL_GetTicks();
    actor->currentSprite = actor->sprites[0];
    SetRenderPosition(actor);

    return actor;
}




void SetPosition(sot_actor_t *actor, vec2 pos) { 
    glm_vec2_copy(pos, actor->position);
    return; 
}

void SetVelocity(sot_actor_t *actor, vec2 pos) { 
    glm_vec2_copy(pos, actor->velocity);
    return; 
}

// Updates the renderRect position based on the the current actor position.
// Position is applied to the center of the sprite.
void SetRenderPosition(sot_actor_t *actor) 
{
    int deltaX = actor->currentSprite->width/2;
    int deltaY = actor->currentSprite->height/2;

    actor->renderRect = (SDL_FRect) {
        .x = actor->position[0]-deltaX,
        .y = actor->position[1]-deltaY,
        .w = 16,
        .h = 16};
}

void RenderActor(sot_actor_t *actor, AppState *appState) {

    sot_sprite_t *animation = actor->currentSprite;

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

    SetRenderPosition(actor);
    SDL_RenderTexture(appState->renderer, animation->atlas, animation->currentFrame->sprite, &(actor->renderRect));

    return;
}


void MoveActor(sot_actor_t *actor, SDL_Event *event) 
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

void UpdateActor(sot_actor_t *actor, float deltaTime) {

    if (actor == NULL)
        return;

    if (actor->applyGravity)
        actor->direction = FALL;
    
    switch (actor->direction)
    {
        case MOVE_RIGHT:
            if (actor->currentSprite != actor->sprites[2]) actor->currentSprite = actor->sprites[2];
            actor->position[0] += actor->velocity[0] * deltaTime;
            break;
        case MOVE_LEFT:
            if (actor->currentSprite != actor->sprites[1]) actor->currentSprite = actor->sprites[1];
            actor->position[0] -= actor->velocity[0] * deltaTime;
            break;
        case FALL:
            actor->renderRect.y += 80 * deltaTime;
            break;
        case IDLE:
            if (actor->currentSprite != actor->sprites[0]) actor->currentSprite = actor->sprites[0];
        default:
            break;
    }
}

void DestroyActor(sot_actor_t *actor) { 
    if (actor == NULL) return;
    
    for (int i = 0; actor->sprites[i] != NULL; i++) {
        DestroySprite(actor->sprites[i]);
    }

    free(actor);
    return; 
}