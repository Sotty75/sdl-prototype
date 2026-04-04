#include "sot_physics.h"
#include "sot_tilemap.h"
#include <SDL3/SDL.h>

// ---- World lifecycle ----

void SOT_Physics_CreateWorld(SOT_PhysicsWorld *world, float gravityX, float gravityY)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){ gravityX, gravityY };
    worldDef.enableSleep = true;
    worldDef.enableContinuous = true;

    world->worldId = b2CreateWorld(&worldDef);
    world->gravity = worldDef.gravity;
    world->timeStep = 1.0f / 60.0f;
    world->accumulator = 0.0f;

    SDL_Log("SOT_Physics: World created (gravity: %.1f, %.1f m/s²)", gravityX, gravityY);
}

void SOT_Physics_DestroyWorld(SOT_PhysicsWorld *world)
{
    if (B2_IS_NON_NULL(world->worldId)) {
        b2DestroyWorld(world->worldId);
        world->worldId = b2_nullWorldId;
    }
}

// ---- Fixed timestep stepping ----

void SOT_Physics_Step(SOT_PhysicsWorld *world, float deltaTime)
{
    // Clamp large delta to avoid spiral of death
    if (deltaTime > 0.25f)
        deltaTime = 0.25f;

    world->accumulator += deltaTime;

    while (world->accumulator >= world->timeStep) {
        b2World_Step(world->worldId, world->timeStep, 4);
        world->accumulator -= world->timeStep;
    }
}

// ---- Body creation helpers ----

b2BodyId SOT_Physics_CreateStaticBody(SOT_PhysicsWorld *world, float px, float py)
{
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = SOT_PixelsToMeters(px, py);
    return b2CreateBody(world->worldId, &bodyDef);
}

b2BodyId SOT_Physics_CreateDynamicBody(SOT_PhysicsWorld *world, float px, float py, bool fixedRotation)
{
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = SOT_PixelsToMeters(px, py);
    bodyDef.fixedRotation = fixedRotation;
    return b2CreateBody(world->worldId, &bodyDef);
}

b2BodyId SOT_Physics_CreateKinematicBody(SOT_PhysicsWorld *world, float px, float py)
{
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_kinematicBody;
    bodyDef.position = SOT_PixelsToMeters(px, py);
    return b2CreateBody(world->worldId, &bodyDef);
}

// ---- Shape creation helpers ----

b2ShapeId SOT_Physics_AddBoxShape(b2BodyId bodyId, float halfW, float halfH,
                                   float offsetX, float offsetY,
                                   b2Filter filter, bool isSensor)
{
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter = filter;
    shapeDef.isSensor = isSensor;
    shapeDef.enableSensorEvents = isSensor;
    shapeDef.enableContactEvents = !isSensor;

    float hw = halfW * SOT_METERS_PER_PIXEL;
    float hh = halfH * SOT_METERS_PER_PIXEL;
    b2Polygon box = b2MakeOffsetBox(hw, hh,
        (b2Vec2){ offsetX * SOT_METERS_PER_PIXEL, offsetY * SOT_METERS_PER_PIXEL },
        b2MakeRot(0.0f));

    return b2CreatePolygonShape(bodyId, &shapeDef, &box);
}

b2ShapeId SOT_Physics_AddCircleShape(b2BodyId bodyId, float radius,
                                      float offsetX, float offsetY,
                                      b2Filter filter, bool isSensor)
{
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter = filter;
    shapeDef.isSensor = isSensor;
    shapeDef.enableSensorEvents = isSensor;
    shapeDef.enableContactEvents = !isSensor;

    b2Circle circle = {
        .center = { offsetX * SOT_METERS_PER_PIXEL, offsetY * SOT_METERS_PER_PIXEL },
        .radius = radius * SOT_METERS_PER_PIXEL,
    };

    return b2CreateCircleShape(bodyId, &shapeDef, &circle);
}

b2ShapeId SOT_Physics_AddSegmentShape(b2BodyId bodyId,
                                       float x1, float y1, float x2, float y2,
                                       b2Filter filter)
{
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter = filter;
    shapeDef.enableContactEvents = true;

    b2Segment segment = {
        .point1 = { x1 * SOT_METERS_PER_PIXEL, y1 * SOT_METERS_PER_PIXEL },
        .point2 = { x2 * SOT_METERS_PER_PIXEL, y2 * SOT_METERS_PER_PIXEL },
    };

    return b2CreateSegmentShape(bodyId, &shapeDef, &segment);
}

// ---- Queries ----

// Callback context for point query
typedef struct {
    b2BodyId result;
} PointQueryContext;

static bool PointQueryCallback(b2ShapeId shapeId, void *context)
{
    PointQueryContext *ctx = (PointQueryContext *)context;
    ctx->result = b2Shape_GetBody(shapeId);
    return false; // Stop after first hit
}

b2BodyId SOT_Physics_PointQuery(SOT_PhysicsWorld *world, float px, float py, uint64_t maskBits)
{
    b2Vec2 point = SOT_PixelsToMeters(px, py);
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = maskBits;

    PointQueryContext ctx = { .result = b2_nullBodyId };

    // Use a tiny AABB centered on the point
    float e = 0.001f;
    b2AABB aabb = { { point.x - e, point.y - e }, { point.x + e, point.y + e } };
    b2World_OverlapAABB(world->worldId, aabb, filter, PointQueryCallback, &ctx);
    return ctx.result;
}

// Callback context for raycast
typedef struct {
    b2BodyId result;
    float fraction;
    b2Vec2 normal;
} RaycastContext;

static float RaycastCallback(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void *context)
{
    RaycastContext *ctx = (RaycastContext *)context;
    ctx->result = b2Shape_GetBody(shapeId);
    ctx->fraction = fraction;
    ctx->normal = normal;
    return fraction; // Clip ray to this hit (find closest)
}

b2BodyId SOT_Physics_Raycast(SOT_PhysicsWorld *world,
                              float px1, float py1, float px2, float py2,
                              uint64_t maskBits, float *outFraction, b2Vec2 *outNormal)
{
    b2Vec2 origin = SOT_PixelsToMeters(px1, py1);
    b2Vec2 target = SOT_PixelsToMeters(px2, py2);
    b2Vec2 translation = { target.x - origin.x, target.y - origin.y };

    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = maskBits;

    RaycastContext ctx = { .result = b2_nullBodyId, .fraction = 1.0f, .normal = {0, 0} };
    b2World_CastRay(world->worldId, origin, translation, filter, RaycastCallback, &ctx);

    if (outFraction) *outFraction = ctx.fraction;
    if (outNormal) *outNormal = ctx.normal;
    return ctx.result;
}

// ---- Tilemap integration ----

void SOT_Physics_CreateTilemapBodies(SOT_PhysicsWorld *world, sot_tilemap *tilemap)
{
    if (tilemap == NULL || tilemap->colliders == NULL) return;

    b2Filter terrainFilter = b2DefaultFilter();
    terrainFilter.categoryBits = SOT_CAT_TERRAIN;

    int count = 0;
    sot_collider_node_t *node = tilemap->colliders;

    while (node != NULL) {
        sot_collider_t *col = node->collider;
        if (col == NULL) { node = node->next; continue; }

        switch (col->type) {
            case C2_TYPE_AABB: {
                // cute_c2 AABB: min/max in pixels (Y already negated in loader)
                float cx = (col->shape.AABB.min.x + col->shape.AABB.max.x) * 0.5f;
                float cy = (col->shape.AABB.min.y + col->shape.AABB.max.y) * 0.5f;
                float hw = (col->shape.AABB.max.x - col->shape.AABB.min.x) * 0.5f;
                float hh = (col->shape.AABB.max.y - col->shape.AABB.min.y) * 0.5f;
                if (hh < 0) hh = -hh; // in case min.y > max.y due to Y flip

                b2BodyId body = SOT_Physics_CreateStaticBody(world, cx, cy);
                SOT_Physics_AddBoxShape(body, hw, hh, 0, 0, terrainFilter, false);
                count++;
                break;
            }
            case C2_TYPE_POLY: {
                // Create a static body at the polygon centroid
                float cx = 0, cy = 0;
                for (int i = 0; i < col->shape.poly.count; i++) {
                    cx += col->shape.poly.verts[i].x;
                    cy += col->shape.poly.verts[i].y;
                }
                cx /= col->shape.poly.count;
                cy /= col->shape.poly.count;

                b2BodyId body = SOT_Physics_CreateStaticBody(world, cx, cy);

                // Convert cute_c2 polygon vertices to Box2D (relative to body center, in meters)
                b2Vec2 points[B2_MAX_POLYGON_VERTICES];
                int vertCount = col->shape.poly.count;
                if (vertCount > B2_MAX_POLYGON_VERTICES)
                    vertCount = B2_MAX_POLYGON_VERTICES;

                for (int i = 0; i < vertCount; i++) {
                    points[i] = (b2Vec2){
                        (col->shape.poly.verts[i].x - cx) * SOT_METERS_PER_PIXEL,
                        (col->shape.poly.verts[i].y - cy) * SOT_METERS_PER_PIXEL,
                    };
                }

                b2Hull hull = b2ComputeHull(points, vertCount);
                if (hull.count > 0) {
                    b2Polygon polygon = b2MakePolygon(&hull, 0.0f);
                    b2ShapeDef shapeDef = b2DefaultShapeDef();
                    shapeDef.filter = terrainFilter;
                    shapeDef.enableContactEvents = true;
                    b2CreatePolygonShape(body, &shapeDef, &polygon);
                }
                count++;
                break;
            }
            default:
                break;
        }

        node = node->next;
    }

    SDL_Log("SOT_Physics: Created %d static bodies from tilemap colliders", count);
}
