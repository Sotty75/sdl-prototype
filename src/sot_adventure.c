/*
 * SOT Adventure Game System
 *
 * Point-and-click adventure game mechanics: verb bar, inventory, hover detection,
 * walk-to pathfinding, and object interaction. Designed for Monkey Island / Day
 * of the Tentacle style games.
 */

#include "sot_adventure.h"
#include "sot_scene.h"
#include "sot_actor.h"
#include "sot_physics.h"
#include <string.h>
#include <math.h>

// ---- Verb names ----

static const char *s_verbNames[SOT_VERB_COUNT] = {
    "None",
    "Look at",
    "Pick up",
    "Use",
    "Talk to",
    "Open",
    "Close",
    "Push",
    "Pull",
};

// ---- Lifecycle ----

void SOT_Adventure_Init(SOT_Adventure *adv)
{
    SDL_memset(adv, 0, sizeof(*adv));
    adv->currentVerb = SOT_VERB_LOOK;
    adv->selectedItem = -1;
    adv->hoveredActorIndex = -1;
    adv->walkSpeed = 40.0f;
    adv->initialized = true;
}

void SOT_Adventure_Shutdown(SOT_Adventure *adv)
{
    adv->initialized = false;
}

// ---- Verb ----

void SOT_Adventure_SetVerb(SOT_Adventure *adv, SOT_Verb verb)
{
    if (verb >= 0 && verb < SOT_VERB_COUNT)
        adv->currentVerb = verb;
}

SOT_Verb SOT_Adventure_GetVerb(const SOT_Adventure *adv)
{
    return adv->currentVerb;
}

const char *SOT_Adventure_GetVerbName(SOT_Verb verb)
{
    if (verb >= 0 && verb < SOT_VERB_COUNT)
        return s_verbNames[verb];
    return "Unknown";
}

// ---- Inventory ----

bool SOT_Adventure_AddItem(SOT_Adventure *adv, const char *id, const char *name)
{
    if (adv->inventoryCount >= SOT_INVENTORY_MAX)
        return false;

    // Check if already exists
    for (int i = 0; i < adv->inventoryCount; i++) {
        if (SDL_strcmp(adv->inventory[i].id, id) == 0)
            return true; // Already have it
    }

    SOT_InventoryItem *item = &adv->inventory[adv->inventoryCount];
    SDL_strlcpy(item->id, id, sizeof(item->id));
    SDL_strlcpy(item->name, name, sizeof(item->name));
    adv->inventoryCount++;
    return true;
}

bool SOT_Adventure_RemoveItem(SOT_Adventure *adv, const char *id)
{
    for (int i = 0; i < adv->inventoryCount; i++) {
        if (SDL_strcmp(adv->inventory[i].id, id) == 0) {
            // Shift remaining items down
            for (int j = i; j < adv->inventoryCount - 1; j++)
                adv->inventory[j] = adv->inventory[j + 1];
            adv->inventoryCount--;
            if (adv->selectedItem >= adv->inventoryCount)
                adv->selectedItem = adv->inventoryCount - 1;
            return true;
        }
    }
    return false;
}

bool SOT_Adventure_HasItem(const SOT_Adventure *adv, const char *id)
{
    for (int i = 0; i < adv->inventoryCount; i++) {
        if (SDL_strcmp(adv->inventory[i].id, id) == 0)
            return true;
    }
    return false;
}

void SOT_Adventure_SelectItem(SOT_Adventure *adv, int index)
{
    if (index >= -1 && index < adv->inventoryCount)
        adv->selectedItem = index;
}

// ---- Walkable area ----

void SOT_Adventure_SetWalkableArea(SOT_Adventure *adv,
                                    const float (*vertices)[2], int count)
{
    if (count > 64) count = 64;
    adv->walkableVertexCount = count;
    for (int i = 0; i < count; i++) {
        adv->walkableArea[i][0] = vertices[i][0];
        adv->walkableArea[i][1] = vertices[i][1];
    }
}

bool SOT_Adventure_IsPointWalkable(const SOT_Adventure *adv, float x, float y)
{
    if (adv->walkableVertexCount < 3)
        return true; // No walkable area defined; allow everywhere

    // Ray-casting point-in-polygon test
    int n = adv->walkableVertexCount;
    bool inside = false;

    for (int i = 0, j = n - 1; i < n; j = i++) {
        float xi = adv->walkableArea[i][0], yi = adv->walkableArea[i][1];
        float xj = adv->walkableArea[j][0], yj = adv->walkableArea[j][1];

        if (((yi > y) != (yj > y)) &&
            (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }

    return inside;
}

int SOT_Adventure_ComputeWalkPath(SOT_Adventure *adv,
                                   float startX, float startY,
                                   float endX, float endY)
{
    // Simple direct path for now. If the end point is walkable, go directly.
    // A full nav-mesh solution would be T2+.
    if (SOT_Adventure_IsPointWalkable(adv, endX, endY)) {
        adv->walkPath[0][0] = endX;
        adv->walkPath[0][1] = endY;
        adv->walkPathCount = 1;
        adv->walkPathCurrent = 0;
        return 1;
    }

    // End point not walkable: find closest walkable point on polygon edge
    float bestX = endX, bestY = endY;
    float bestDist = 1e10f;
    int n = adv->walkableVertexCount;

    for (int i = 0, j = n - 1; i < n; j = i++) {
        float ax = adv->walkableArea[j][0], ay = adv->walkableArea[j][1];
        float bx = adv->walkableArea[i][0], by = adv->walkableArea[i][1];

        // Project point onto edge segment
        float dx = bx - ax, dy = by - ay;
        float len2 = dx * dx + dy * dy;
        if (len2 < 0.001f) continue;

        float t = ((endX - ax) * dx + (endY - ay) * dy) / len2;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        float cx = ax + t * dx;
        float cy = ay + t * dy;
        float d = (endX - cx) * (endX - cx) + (endY - cy) * (endY - cy);

        if (d < bestDist) {
            bestDist = d;
            bestX = cx;
            bestY = cy;
        }
    }

    adv->walkPath[0][0] = bestX;
    adv->walkPath[0][1] = bestY;
    adv->walkPathCount = 1;
    adv->walkPathCurrent = 0;
    return 1;
}

// ---- Per-frame update ----

void SOT_Adventure_Update(SOT_Adventure *adv,
                           SOT_Scene *scene,
                           SOT_PhysicsWorld *world,
                           float mouseGameX, float mouseGameY,
                           bool mouseClicked,
                           float deltaTime)
{
    if (!adv->initialized) return;

    // ---- Hover detection ----
    adv->hoveredActorIndex = -1;
    adv->hoveredObjectName[0] = '\0';

    // Use physics point query to find actor under cursor
    b2BodyId hitBody = SOT_Physics_PointQuery(world, mouseGameX, mouseGameY,
                                               SOT_CAT_NPC | SOT_CAT_COLLECTIBLE | SOT_CAT_TRIGGER);

    if (b2Body_IsValid(hitBody)) {
        // Find which actor owns this body
        for (int i = 0; i < scene->actorsCount; i++) {
            SOT_Actor *actor = &scene->actors[i];
            if (!actor->enabled) continue;

            if (B2_ID_EQUALS(actor->bodyId, hitBody)) {
                adv->hoveredActorIndex = i;
                if (actor->actorName[0])
                    SDL_strlcpy(adv->hoveredObjectName, actor->actorName,
                                sizeof(adv->hoveredObjectName));
                break;
            }
        }
    }

    // ---- Click interaction ----
    if (mouseClicked && adv->hoveredActorIndex >= 0) {
        // The Lua callback system handles the actual interaction.
        // The adventure system just provides the verb + target info.
        // Lua scripts check adv->currentVerb and adv->hoveredActorIndex.
        // (Dispatched via on_interact callback in the actor's Lua script)
    }

    // ---- Walk-to: if clicked on empty ground, walk the player there ----
    if (mouseClicked && adv->hoveredActorIndex < 0) {
        // Find the player actor (tagged "player")
        for (int i = 0; i < scene->actorsCount; i++) {
            SOT_Actor *actor = &scene->actors[i];
            if (!actor->enabled) continue;
            if (SOT_Actor_HasTag(actor, "player")) {
                float startX = actor->transform.position[0];
                float startY = actor->transform.position[1];
                SOT_Adventure_ComputeWalkPath(adv, startX, startY,
                                               mouseGameX, mouseGameY);
                break;
            }
        }
    }

    // ---- Walk-to movement ----
    if (adv->walkPathCount > 0 && adv->walkPathCurrent < adv->walkPathCount) {
        // Find player actor
        for (int i = 0; i < scene->actorsCount; i++) {
            SOT_Actor *actor = &scene->actors[i];
            if (!actor->enabled) continue;
            if (!SOT_Actor_HasTag(actor, "player")) continue;

            float tx = adv->walkPath[adv->walkPathCurrent][0];
            float ty = adv->walkPath[adv->walkPathCurrent][1];
            float dx = tx - actor->transform.position[0];
            float dy = ty - actor->transform.position[1];
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < 2.0f) {
                // Reached waypoint
                adv->walkPathCurrent++;
                if (adv->walkPathCurrent >= adv->walkPathCount) {
                    adv->walkPathCount = 0;
                    // Stop the actor
                    if (b2Body_IsValid(actor->bodyId)) {
                        b2Body_SetLinearVelocity(actor->bodyId, (b2Vec2){0, 0});
                    }
                }
            } else {
                // Move toward waypoint
                float speed = adv->walkSpeed * SOT_METERS_PER_PIXEL;
                float nx = dx / dist;
                float ny = dy / dist;
                if (b2Body_IsValid(actor->bodyId)) {
                    b2Body_SetLinearVelocity(actor->bodyId,
                        (b2Vec2){ nx * speed, ny * speed });
                }
            }
            break;
        }
    }
}
