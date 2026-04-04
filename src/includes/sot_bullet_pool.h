#ifndef SOT_BULLET_POOL_H_
#define SOT_BULLET_POOL_H_

#include <stdbool.h>
#include "cglm.h"
#include "sot_physics.h"

// Maximum bullets per pool
#define SOT_BULLET_POOL_MAX 512

// ---- Bullet instance ----

typedef struct SOT_Bullet {
    bool active;
    float x, y;                 // Position in pixels
    float vx, vy;               // Velocity in pixels/sec
    float lifetime;             // Remaining lifetime in seconds (0 = infinite)
    float age;                  // Time alive
    uint32_t atlasIndex;        // Texture atlas index for rendering
    ivec2 frameCoords;          // Sprite frame in atlas
    ivec2 frameSize;            // Sprite frame size
    ivec2 atlasSize;            // Atlas texture size
    uint64_t categoryBits;      // Collision filter
    uint64_t maskBits;
} SOT_Bullet;

// ---- Bullet pattern definition ----

typedef enum {
    SOT_BULLET_PATTERN_LINE,      // Single direction
    SOT_BULLET_PATTERN_SPREAD,    // Fan of bullets
    SOT_BULLET_PATTERN_RING,      // Full circle
    SOT_BULLET_PATTERN_AIMED,     // Aimed at a target position
} SOT_BulletPatternType;

typedef struct SOT_BulletPattern {
    SOT_BulletPatternType type;
    float speed;                // Pixels/sec
    float lifetime;             // Seconds (0 = no limit, removed at screen edge)
    int count;                  // Number of bullets per burst
    float spreadAngle;          // For SPREAD: total angle in degrees
    float angleOffset;          // Base angle offset in degrees
    float targetX, targetY;     // For AIMED: target position in pixels

    // Sprite info (shared for all bullets in pattern)
    uint32_t atlasIndex;
    ivec2 frameCoords;
    ivec2 frameSize;
    ivec2 atlasSize;
    uint64_t categoryBits;
    uint64_t maskBits;
} SOT_BulletPattern;

// ---- Bullet pool ----

typedef struct SOT_BulletPool {
    SOT_Bullet bullets[SOT_BULLET_POOL_MAX];
    int activeCount;

    // Screen bounds for culling (pixels)
    float screenMinX, screenMinY;
    float screenMaxX, screenMaxY;
} SOT_BulletPool;

// ---- API ----

void SOT_BulletPool_Init(SOT_BulletPool *pool);

// Spawn a single bullet at position with velocity
int SOT_BulletPool_Spawn(SOT_BulletPool *pool,
                          float x, float y, float vx, float vy,
                          float lifetime);

// Spawn bullets from a pattern definition at origin position
void SOT_BulletPool_SpawnPattern(SOT_BulletPool *pool,
                                  float originX, float originY,
                                  const SOT_BulletPattern *pattern);

// Update all active bullets (move, age, cull)
void SOT_BulletPool_Update(SOT_BulletPool *pool, float deltaTime);

// Set screen bounds for off-screen culling
void SOT_BulletPool_SetBounds(SOT_BulletPool *pool,
                               float minX, float minY, float maxX, float maxY);

// Get count of active bullets
int SOT_BulletPool_ActiveCount(const SOT_BulletPool *pool);

// Deactivate all bullets
void SOT_BulletPool_Clear(SOT_BulletPool *pool);

// Check collision between a bullet and a point (radius-based).
// Returns index of first colliding bullet, or -1.
int SOT_BulletPool_CheckCollision(const SOT_BulletPool *pool,
                                   float px, float py, float radius,
                                   uint64_t targetCategory);

// Deactivate a specific bullet by index
void SOT_BulletPool_Kill(SOT_BulletPool *pool, int index);

#endif
