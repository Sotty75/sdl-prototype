#include "test_main.h"

sot_world *TEST_CreateWorld(int size) 
{
    sot_quad * template = sot_quad_create();
    sot_world *world = (sot_world*) malloc(sizeof(sot_world));
    
    world->quadsArray = (sot_quad*) malloc((size) * sizeof(sot_quad));
    world->size = size;

    // fill the world array
    for (int i = 0; i < size; i++) { sot_quad_copy(template, &(world->quadsArray[i])); }

    // we place four quads in four distant corners (index 0 stays in the world origin)
    glm_vec3_copy((vec3) {1,1,0}, world->quadsArray[1].position);
    glm_vec3_copy((vec3) {1,-1,0}, world->quadsArray[2].position);
    glm_vec3_copy((vec3) {-1,1,0}, world->quadsArray[3].position);
    glm_vec3_copy((vec3) {-1,-1,0}, world->quadsArray[4].position);

    // we place four quads in four distant corners (index 0 stays in the world origin)
    glm_vec3_copy((vec3) {5,3,0}, world->quadsArray[5].position);
    glm_vec3_copy((vec3) {5,-3,0}, world->quadsArray[6].position);
    glm_vec3_copy((vec3) {-5,3,0}, world->quadsArray[7].position);
    glm_vec3_copy((vec3) {-5,-3,0}, world->quadsArray[8].position);

    world->vertexBufferSize = size * sizeof(world->quadsArray[0].verts);
    world->indexBufferSize = size * sizeof(world->quadsArray[0].indexes);
}