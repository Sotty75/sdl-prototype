#ifndef SOT_QUAD_H_
#define SOT_QUAD_H_

// Define constants
#define QUAD_VERTS 4
#define QUAD_INDEXES 6

// include common type definitions
#include "sot_common.h"

typedef struct sot_quad {
    vertex verts[QUAD_VERTS];
    uint16_t indexes[QUAD_INDEXES];
} sot_quad;

sot_quad sot_quad_create();

#endif