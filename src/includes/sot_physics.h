#ifndef SOT_PHYSICS_H_
#define SOT_PHYSICS_H_

#include "cglm.h"
#include <box2d/box2d.h>

// ---- Coordinate conversion ----
// All game rendering uses pixels. Box2D uses meters.
// This constant defines how many pixels equal 1 meter in Box2D.
#define SOT_PIXELS_PER_METER 16.0f
#define SOT_METERS_PER_PIXEL (1.0f / SOT_PIXELS_PER_METER)

// Convert between pixel and meter coordinate systems
static inline b2Vec2 SOT_PixelsToMeters(float px, float py) {
    return (b2Vec2){ px * SOT_METERS_PER_PIXEL, py * SOT_METERS_PER_PIXEL };
}

static inline void SOT_MetersToPixels(b2Vec2 m, float *px, float *py) {
    *px = m.x * SOT_PIXELS_PER_METER;
    *py = m.y * SOT_PIXELS_PER_METER;
}

// ---- Collision categories (bitmask) ----
#define SOT_CAT_TERRAIN        (1ULL << 0)
#define SOT_CAT_PLAYER         (1ULL << 1)
#define SOT_CAT_ENEMY          (1ULL << 2)
#define SOT_CAT_PLAYER_BULLET  (1ULL << 3)
#define SOT_CAT_ENEMY_BULLET   (1ULL << 4)
#define SOT_CAT_TRIGGER        (1ULL << 5)
#define SOT_CAT_COLLECTIBLE    (1ULL << 6)
#define SOT_CAT_NPC            (1ULL << 7)

// ---- Legacy transform (still used by rendering) ----
typedef struct SOT_Transform {
    vec2 position;
    vec2 scale;
    vec2 rotation;
} SOT_Transform;

// ---- Physics world state ----
typedef struct SOT_PhysicsWorld {
    b2WorldId worldId;
    float timeStep;         // Fixed timestep for physics (default 1/60)
    float accumulator;      // Time accumulator for fixed-step integration
    b2Vec2 gravity;         // Gravity vector in meters/s²
} SOT_PhysicsWorld;

// ---- World lifecycle ----
void SOT_Physics_CreateWorld(SOT_PhysicsWorld *world, float gravityX, float gravityY);
void SOT_Physics_DestroyWorld(SOT_PhysicsWorld *world);

// Step the physics world. Call with frame deltaTime; internally uses fixed timestep.
void SOT_Physics_Step(SOT_PhysicsWorld *world, float deltaTime);

// ---- Body creation helpers ----

// Create a static body at the given pixel position
b2BodyId SOT_Physics_CreateStaticBody(SOT_PhysicsWorld *world, float px, float py);

// Create a dynamic body at the given pixel position
b2BodyId SOT_Physics_CreateDynamicBody(SOT_PhysicsWorld *world, float px, float py, bool fixedRotation);

// Create a kinematic body at the given pixel position
b2BodyId SOT_Physics_CreateKinematicBody(SOT_PhysicsWorld *world, float px, float py);

// ---- Shape creation helpers (dimensions in pixels) ----

// Add a box shape to a body. halfW/halfH in pixels, offset from body center in pixels.
b2ShapeId SOT_Physics_AddBoxShape(b2BodyId bodyId, float halfW, float halfH,
                                   float offsetX, float offsetY,
                                   b2Filter filter, bool isSensor);

// Add a circle shape to a body. radius in pixels, offset from body center in pixels.
b2ShapeId SOT_Physics_AddCircleShape(b2BodyId bodyId, float radius,
                                      float offsetX, float offsetY,
                                      b2Filter filter, bool isSensor);

// Add a segment (edge) shape to a body. Points in pixels relative to body.
b2ShapeId SOT_Physics_AddSegmentShape(b2BodyId bodyId,
                                       float x1, float y1, float x2, float y2,
                                       b2Filter filter);

// ---- Tilemap integration ----
struct sot_tilemap;
void SOT_Physics_CreateTilemapBodies(SOT_PhysicsWorld *world, struct sot_tilemap *tilemap);

// ---- Queries (coordinates in pixels) ----

// Point query: returns the first body whose shape contains the given pixel point.
// Returns b2_nullBodyId if nothing found.
b2BodyId SOT_Physics_PointQuery(SOT_PhysicsWorld *world, float px, float py, uint64_t maskBits);

// Raycast: cast from (px1,py1) to (px2,py2) in pixels. Returns closest hit.
// fraction is 0..1 along the ray. Returns b2_nullBodyId if no hit.
b2BodyId SOT_Physics_Raycast(SOT_PhysicsWorld *world,
                              float px1, float py1, float px2, float py2,
                              uint64_t maskBits, float *outFraction, b2Vec2 *outNormal);

#endif
