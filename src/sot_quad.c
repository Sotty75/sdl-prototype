#include "sot_quad.h"
#include "cglm.h"

sot_quad *sot_quad_create() {
    sot_quad *q = (sot_quad*)malloc(sizeof(sot_quad));

    // Initialize QUAD vertices on the origin, sides size = 1
    q->verts[0] = (vertex) {{-0.5,-0.5,0},{0,1,0},{0,1}};     // v0
    q->verts[1] = (vertex) {{0.5,-0.5,0},{0,0,1},{1, 1.0}};   // v1
    q->verts[2] = (vertex) {{0.5,0.5,0},{1,0,0},{1,0}};       // v2
    q->verts[3] = (vertex) {{-0.5,0.5,0},{1,0,0},{0,0}};      // v3

    // Initialize Indexes for the rendering of two triangles in the Index buffer
    q->indexes[0] =  0;
    q->indexes[1] =  1;
    q->indexes[2] =  2;
    q->indexes[3] =  0;
    q->indexes[4] =  2;
    q->indexes[5] =  3;

    // set the initial transform to the identity matrix
    glm_vec3_copy((vec3) {0,0,1}, q->rotationAxis);
    glm_vec3_zero(q->position);
    q->rotation = 0;
    q->scale = 1;

    // return the address to the quad stored on the heap
    return q;
}

/* Copy a quad structure to a destination entity. */
void sot_quad_copy(sot_quad *source, sot_quad *dest) {
    
     // Initialize destUAD vertices on the origin, sides size = 1
    dest->verts[0] = source->verts[0];
    dest->verts[1] = source->verts[1];
    dest->verts[2] = source->verts[2];
    dest->verts[3] = source->verts[3];

    // Initialize Indexes for the endering of two triangles in the Index buffer
    dest->indexes[0] =  source->indexes[0];
    dest->indexes[1] =  source->indexes[1];
    dest->indexes[2] =  source->indexes[2];
    dest->indexes[3] =  source->indexes[3];
    dest->indexes[4] =  source->indexes[4];
    dest->indexes[5] =  source->indexes[5];

    // set the initial transform to the identity matrix
    glm_vec3_copy(source->rotationAxis, dest->rotationAxis);
    glm_vec3_copy(source->position, dest->position);
    dest->rotation =  source->rotation ;
    dest->scale = source->scale;
}


// Replace the model transform with a new one forced from the outside
void sot_quad_get_transform_RT(mat4 transform, sot_quad* q) {
    glm_mat4_identity(transform);
    glm_mat4_scale(transform, q->scale);
    glm_translate(transform, q->position);
    glm_rotate(transform, q->rotation, q->rotationAxis);
    
    return;
}

void sot_quad_get_transform_TR(mat4 transform, sot_quad* q) {
    glm_mat4_identity(transform);
    glm_mat4_scale(transform, q->scale);
    glm_rotate(transform, q->rotation, q->rotationAxis);
    glm_translate(transform, q->position);
    
    return;
}


void sot_quad_position(sot_quad* q, vec3 position) {
    glm_vec3_add(position, q->position, q->position);
}

void sot_quad_rotation(sot_quad* q, float rotation) {
    q->rotation += rotation;
}

void sot_quad_scale(sot_quad* q, float scale) {
    q->scale += scale;
}

// Apply a rotation to the current model transform

void sot_quad_free(sot_quad **q) {
    free(*q);
    *q = NULL;
}
