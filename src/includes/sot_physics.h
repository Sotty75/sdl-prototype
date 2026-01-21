#ifndef SOT_PHYSICS_H_
#define SOT_PHYSICS_H_

#include "cglm.h"

/**
 * @brief Defines the physical behavior type of an object.
 * Used to determine how the physics engine treats the object during simulation.
 */
typedef enum SOT_PHYSICS_BODY {
    SOT_PHB_NONE           = 0,
    SOT_PHB_STATIC         = 1 << 0,  // 1
    SOT_PHB_KINEMATIC      = 1 << 1,  // 2
    SOT_PHB_DYNAMIC        = 1 << 2,  // 4
} SOT_PHYSICS_BODY;

/**
 * @brief Represents the spatial transformation of an object.
 * Stores position, scale, and rotation vectors.
 */
typedef struct SOT_Transform {
    vec2 position;
    vec2 scale;
    vec2 rotation;
} SOT_Transform;

/**
 * @brief Encapsulates physics properties for an object.
 * Includes the body type classification and velocity components (magnitude and direction).
 */
typedef struct SOT_Physics {
    SOT_PHYSICS_BODY body;
    float v_magnitude;
    vec2 v_direction;
} SOT_Physics;

#endif