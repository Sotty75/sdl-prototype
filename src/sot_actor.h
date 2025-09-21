#include <stdlib.h>
#include "cglm.h"
#include "sprites.h"

#ifndef ACTOR_H
#define ACTOR_H


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
    Sprite *anims;
} Actor;

Actor *CreateActor(char *name, vec2 pos, vec2 vel, Sprite *anims);
void SetPosition(vec2 pos);
void Move(Directions direction);
void DestroyActor(Actor *actor);

#endif // ACTOR_H
