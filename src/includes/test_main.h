#ifndef TEST_MAIN_H_
#define TEST_MAIN_H_

#include "sot_quad.h"

typedef struct sot_world {
    sot_quad *quadsArray;
    int size;
    int vertexDataSize;
    int indexDataSize;
} sot_world;

sot_world* TEST_CreateWorld(int size);

#endif