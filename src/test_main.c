#include "test_main.h"

sot_quad sot_world[255];


void TEST_CreateWorld() {
    sot_quad * template = sot_quad_create();
    
    // fill the world array
    for (int i = 0; i < 10; i++) { sot_quad_copy(template, &sot_world[i]); }

    // we place four quads in four distant corners (index 0 stays in the world origin)
    glm_vec3_copy((vec3) {1,1,0}, sot_world[1].position);
    glm_vec3_copy((vec3) {1,-1,0}, sot_world[2].position);
    glm_vec3_copy((vec3) {-1,13,0}, sot_world[3].position);
    glm_vec3_copy((vec3) {-1,-1,0}, sot_world[4].position);

    // we place four quads in four distant corners (index 0 stays in the world origin)
    glm_vec3_copy((vec3) {5,3,0}, sot_world[5].position);
    glm_vec3_copy((vec3) {5,-3,0}, sot_world[6].position);
    glm_vec3_copy((vec3) {-5,-3,0}, sot_world[7].position);
    glm_vec3_copy((vec3) {-5,-3,0}, sot_world[8].position);
}