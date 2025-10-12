#include <stdlib.h>
#include "cute_c2.h"

typedef struct sot_collider_t {
    c2Circle circle;
    c2Capsule capsule;
    c2AABB AABB;
    c2Poly poly;
    C2_TYPE type;
} sot_collider_t;

typedef struct sot_collider_node_t {
    sot_collider_t collider;
    struct sot_collider_node_t *next;
} sot_collider_node_t;

typedef struct sot_colliders_list_t {
    sot_collider_node_t *colliderNode;
} sot_colliders_list_t;

void AddCollider(sot_colliders_list_t* list, sot_collider_t);
void ClearList(sot_colliders_list_t* list);
void DestroyList(sot_colliders_list_t* list);

