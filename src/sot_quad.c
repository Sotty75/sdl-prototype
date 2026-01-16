#include "sot_quad.h"

sot_quad sot_quad_create() {
    sot_quad q;

    // Initialize QUAD vertices on the origin, sides size = 1
    q.verts[0] = (vertex) {{-0.5,-0.5,0},{0,1,0},{0,1}};     // v0
    q.verts[1] = (vertex) {{0.5,-0.5,0},{0,0,1},{1, 1.0}};   // v1
    q.verts[2] = (vertex) {{0.5,0.5,0},{1,0,0},{1,0}};       // v2
    q.verts[3] = (vertex) {{-0.5,0.5,0},{1,0,0},{0,0}};      // v3

    // Initialize Indexes for the rendering of two triangles in the Index buffer
    q.indexes[0] =  0;
    q.indexes[1] =  1;
    q.indexes[2] =  2;
    q.indexes[3] =  0;
    q.indexes[4] =  2;
    q.indexes[5] =  3;

    // return the address to the quad stored on the heap
    return q;
}