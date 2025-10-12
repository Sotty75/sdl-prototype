#include <stdlib.h>
#include "cute_c2.h"

typedef struct sot_collider_t {
    c2Circle circle;
    c2Capsule capsule;
    c2AABB AABB;
    c2Poly poly;
    C2_TYPE type;
} sot_collider_t;
