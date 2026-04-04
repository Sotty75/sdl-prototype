/*
 * SOT Bullet Pool
 *
 * High-performance bullet management for shmup games. Pre-allocated fixed-size
 * pool with recycling. Uses simple position + velocity (no Box2D bodies) for
 * minimal overhead at high bullet counts.
 */

#include "sot_bullet_pool.h"
#include <math.h>
#include <SDL3/SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void SOT_BulletPool_Init(SOT_BulletPool *pool)
{
    SDL_memset(pool, 0, sizeof(*pool));
    pool->screenMinX = -32.0f;
    pool->screenMinY = -32.0f;
    pool->screenMaxX = 352.0f;  // 320 + margin
    pool->screenMaxY = 272.0f;  // 240 + margin
}

static int FindFreeSlot(SOT_BulletPool *pool)
{
    for (int i = 0; i < SOT_BULLET_POOL_MAX; i++) {
        if (!pool->bullets[i].active)
            return i;
    }
    return -1; // Pool full
}

int SOT_BulletPool_Spawn(SOT_BulletPool *pool,
                          float x, float y, float vx, float vy,
                          float lifetime)
{
    int idx = FindFreeSlot(pool);
    if (idx < 0) return -1;

    SOT_Bullet *b = &pool->bullets[idx];
    b->active = true;
    b->x = x;
    b->y = y;
    b->vx = vx;
    b->vy = vy;
    b->lifetime = lifetime;
    b->age = 0.0f;
    pool->activeCount++;

    return idx;
}

void SOT_BulletPool_SpawnPattern(SOT_BulletPool *pool,
                                  float originX, float originY,
                                  const SOT_BulletPattern *pat)
{
    float baseAngleRad = pat->angleOffset * (float)(M_PI / 180.0);

    for (int i = 0; i < pat->count; i++) {
        float angle = baseAngleRad;

        switch (pat->type) {
            case SOT_BULLET_PATTERN_LINE:
                // All bullets same direction
                break;

            case SOT_BULLET_PATTERN_SPREAD: {
                float spreadRad = pat->spreadAngle * (float)(M_PI / 180.0);
                if (pat->count > 1) {
                    float step = spreadRad / (float)(pat->count - 1);
                    angle = baseAngleRad - spreadRad * 0.5f + step * (float)i;
                }
                break;
            }

            case SOT_BULLET_PATTERN_RING: {
                float step = 2.0f * (float)M_PI / (float)pat->count;
                angle = baseAngleRad + step * (float)i;
                break;
            }

            case SOT_BULLET_PATTERN_AIMED: {
                float dx = pat->targetX - originX;
                float dy = pat->targetY - originY;
                angle = atan2f(dy, dx);
                // For multi-bullet aimed, add spread around aim direction
                if (pat->count > 1 && pat->spreadAngle > 0.0f) {
                    float spreadRad = pat->spreadAngle * (float)(M_PI / 180.0);
                    float step = spreadRad / (float)(pat->count - 1);
                    angle = angle - spreadRad * 0.5f + step * (float)i;
                }
                break;
            }
        }

        float vx = cosf(angle) * pat->speed;
        float vy = sinf(angle) * pat->speed;

        int idx = SOT_BulletPool_Spawn(pool, originX, originY, vx, vy, pat->lifetime);
        if (idx >= 0) {
            SOT_Bullet *b = &pool->bullets[idx];
            b->atlasIndex   = pat->atlasIndex;
            SDL_memcpy(b->frameCoords, pat->frameCoords, sizeof(ivec2));
            SDL_memcpy(b->frameSize,   pat->frameSize,   sizeof(ivec2));
            SDL_memcpy(b->atlasSize,   pat->atlasSize,   sizeof(ivec2));
            b->categoryBits = pat->categoryBits;
            b->maskBits     = pat->maskBits;
        }
    }
}

void SOT_BulletPool_Update(SOT_BulletPool *pool, float dt)
{
    int active = 0;

    for (int i = 0; i < SOT_BULLET_POOL_MAX; i++) {
        SOT_Bullet *b = &pool->bullets[i];
        if (!b->active) continue;

        // Move
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        b->age += dt;

        // Lifetime check
        if (b->lifetime > 0.0f && b->age >= b->lifetime) {
            b->active = false;
            continue;
        }

        // Off-screen culling
        if (b->x < pool->screenMinX || b->x > pool->screenMaxX ||
            b->y < pool->screenMinY || b->y > pool->screenMaxY) {
            b->active = false;
            continue;
        }

        active++;
    }

    pool->activeCount = active;
}

void SOT_BulletPool_SetBounds(SOT_BulletPool *pool,
                               float minX, float minY, float maxX, float maxY)
{
    pool->screenMinX = minX;
    pool->screenMinY = minY;
    pool->screenMaxX = maxX;
    pool->screenMaxY = maxY;
}

int SOT_BulletPool_ActiveCount(const SOT_BulletPool *pool)
{
    return pool->activeCount;
}

void SOT_BulletPool_Clear(SOT_BulletPool *pool)
{
    for (int i = 0; i < SOT_BULLET_POOL_MAX; i++)
        pool->bullets[i].active = false;
    pool->activeCount = 0;
}

int SOT_BulletPool_CheckCollision(const SOT_BulletPool *pool,
                                   float px, float py, float radius,
                                   uint64_t targetCategory)
{
    float r2 = radius * radius;

    for (int i = 0; i < SOT_BULLET_POOL_MAX; i++) {
        const SOT_Bullet *b = &pool->bullets[i];
        if (!b->active) continue;

        // Category filter: bullet's mask must include target's category
        if (targetCategory != 0 && !(b->maskBits & targetCategory))
            continue;

        float dx = b->x - px;
        float dy = b->y - py;
        if (dx * dx + dy * dy <= r2)
            return i;
    }

    return -1;
}

void SOT_BulletPool_Kill(SOT_BulletPool *pool, int index)
{
    if (index >= 0 && index < SOT_BULLET_POOL_MAX) {
        if (pool->bullets[index].active) {
            pool->bullets[index].active = false;
            pool->activeCount--;
        }
    }
}
