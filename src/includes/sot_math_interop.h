#ifndef SOT_MATH_INTEROP_H
#define SOT_MATH_INTEROP_H

#include "cglm.h"
#include "cute_c2.h"

/*
* This header provides simple inline functions to safely convert between
* vector types from cglm and cute_c2.
*
* cglm's vec2 is a float[2].
* cute_c2's c2v is a struct { float x, y; }.
*
* In C, these types have identical memory layouts, so we can safely
* cast pointers between them for zero-cost conversion.
*/

/**
 * @brief Casts a pointer to a cglm vec2 to a pointer to a c2v.
 */
static inline c2v* vec2_to_c2v(vec2* v) {
    return (c2v*)v;
}

/**
 * @brief Casts a pointer to a c2v to a pointer to a cglm vec2.
 */
static inline vec2* c2v_to_vec2(c2v* v) {
    return (vec2*)v;
}

#endif // SOT_MATH_INTEROP_H