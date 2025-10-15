
#ifndef SOT_COLLIDER_H_
#define SOT_COLLIDER_H_

#include <stdlib.h>
#include "cute_c2.h"
#include "appstate.h"

typedef struct sot_collider_t {
    C2_TYPE type;
    union {
        c2Circle circle;
        c2Capsule capsule;
        c2AABB AABB;
        c2Poly poly;
    } shape;
} sot_collider_t;

typedef struct sot_collider_node_t {
    sot_collider_t *collider;
    struct sot_collider_node_t *next;
} sot_collider_node_t;

void AddCollider(sot_collider_node_t* , sot_collider_t *);
void DestroyColliders(sot_collider_node_t* );
void DrawCollidersDebugInfo(sot_collider_t* , AppState* state);

#endif
