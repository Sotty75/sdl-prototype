#include "test_main.h"

sot_world *TEST_CreateWorld(int size) 
{
    sot_quad *template = sot_quad_create();
    sot_world *world = (sot_world*) malloc(sizeof(sot_world));
    
    world->quad = sot_quad_create();
    world->quads = (sot_quad_info*) malloc((size) * sizeof(sot_quad_info));
    world->transforms = (mat4 *) malloc((size) * sizeof(mat4));
    world->count = size;

    // fill the world array
    for (int i = 0; i < size; i++) { sot_quad_info_init(&(world->quads[i])); }

    if (size == 9) {
        // we place four quads in four distant corners (index 0 stays in the world origin)
        glm_vec3_copy((vec3) {1,1,0}, world->quads[1].position);
        glm_vec3_copy((vec3) {1,-1,0}, world->quads[2].position);
        glm_vec3_copy((vec3) {-1,1,0}, world->quads[3].position);
        glm_vec3_copy((vec3) {-1,-1,0}, world->quads[4].position);

        // we place four quads in four distant corners (index 0 stays in the world origin)
        glm_vec3_copy((vec3) {5,3,0}, world->quads[5].position);
        glm_vec3_copy((vec3) {5,-3,0}, world->quads[6].position);
        glm_vec3_copy((vec3) {-5,3,0}, world->quads[7].position);
        glm_vec3_copy((vec3) {-5,-3,0}, world->quads[8].position);
    }

    return world;
}