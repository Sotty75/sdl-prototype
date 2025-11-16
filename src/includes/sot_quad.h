#ifndef SOT_QUAD_H_
#define SOT_QUAD_H_

#include "common.h"

typedef struct sot_quad {
    vertex verts[4];
    uint16_t indexes[6];
    vec3 position;
    vec3 rotationAxis;
    float rotation;
    float scale;
} sot_quad;

sot_quad *sot_create_quad_default();

void sot_quad_get_transform_RT(mat4 transform, sot_quad* q);
void sot_quad_get_transform_TR(mat4 transform, sot_quad* q);
void sot_quad_position(sot_quad* q, vec3 position);
void sot_quad_rotation(sot_quad* q, float rotation);
void sot_quad_scale(sot_quad* q, float scale);


#endif