#include <stdlib.h>
#include "sot_actor.h"
#include "appstate.h"


Actor *CreateActor(char *name, vec2 pos, vec2 vel, Sprite **anims) 
{
    Actor *actor = malloc(sizeof(Actor));
    if (actor == NULL) return NULL;

    actor->name = name;
    glm_vec2_copy(pos, actor->pos);
    glm_vec2_copy(vel, actor->vel);
    actor->anims = anims;
    actor->last_step = SDL_GetTicks();
    actor->currentAnim = actor->anims[0];
    actor->postion = (SDL_FRect){0,0,16,16};

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

    Sprite *animation = actor->currentAnim;

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

    SDL_RenderTexture(appState->renderer, animation->atlas, animation->currentFrame->sprite, &(actor->postion));

    return;
}


void Move(Directions direction) { return; }

void DestroyActor(Actor *actor) { 
    if (actor == NULL) return;
    
    for (int i = 0; actor->anims[i] != NULL; i++) {
        DestroySprite(actor->anims[i]);
    }

    free(actor);
    return; 
}