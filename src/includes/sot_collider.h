
#ifndef SOT_COLLIDER_H_
#define SOT_COLLIDER_H_


#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>
#include "cglm.h"
#include "appstate.h"
#include "cute_c2.h"
#include "sot_gpu_pipeline.h"

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

void AppendCollider(sot_collider_node_t* , sot_collider_t *);
void AppendCollidersList(sot_collider_node_t** , sot_collider_node_t*);
void DestroyColliders(sot_collider_node_t* );
int CollidersCount(sot_collider_node_t* );
void DrawCollidersDebugInfo(SOT_GPU_State *gpu, sot_collider_t pCollider);

#endif
