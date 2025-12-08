#ifndef SOT_QUAD_H_
#define SOT_QUAD_H_

#include "common.h"




typedef struct sot_quad {
    vertex verts[QUAD_VERTS];
    uint16_t indexes[QUAD_INDEXES];
} sot_quad;

typedef struct sot_quad_info {
    vec3 position;
    vec3 rotationAxis;
    float rotation;
    float scale;
    mat4 transform;
    vec2 textureCoords;
} sot_quad_info;

sot_quad *sot_quad_create();
void sot_quad_info_init(sot_quad_info *info);
void sot_quad_copy(sot_quad *source, sot_quad_info *dest);
void sot_quad_get_transform_RT(mat4 transform, sot_quad_info* q);
void sot_quad_get_transform_TR(mat4 transform, sot_quad_info* q);
void sot_quad_position(sot_quad_info* q, vec3 position);
void sot_quad_rotation(sot_quad_info* q, float rotation);
void sot_quad_scale(sot_quad_info* q, float scale);

#endif