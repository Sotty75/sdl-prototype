#ifndef SOT_ADVENTURE_H_
#define SOT_ADVENTURE_H_

#include <stdbool.h>
#include "cglm.h"

// Forward declarations
struct SOT_Actor;
struct SOT_Scene;
struct SOT_Input;
struct SOT_PhysicsWorld;

// ---- Verb actions ----

typedef enum {
    SOT_VERB_NONE = 0,
    SOT_VERB_LOOK,
    SOT_VERB_PICKUP,
    SOT_VERB_USE,
    SOT_VERB_TALK,
    SOT_VERB_OPEN,
    SOT_VERB_CLOSE,
    SOT_VERB_PUSH,
    SOT_VERB_PULL,
    SOT_VERB_COUNT
} SOT_Verb;

// ---- Inventory ----

#define SOT_INVENTORY_MAX 24

typedef struct SOT_InventoryItem {
    char id[64];                // Item identifier (e.g., "rusty_key")
    char name[64];              // Display name (e.g., "Rusty Key")
    uint32_t atlasIndex;        // Texture atlas for icon
    ivec2 iconCoords;           // Icon frame in atlas
    ivec2 iconSize;             // Icon frame size
} SOT_InventoryItem;

// ---- Walk path ----

#define SOT_WALK_PATH_MAX 32

// ---- Adventure game state ----

typedef struct SOT_Adventure {
    bool initialized;

    // Current verb/action mode
    SOT_Verb currentVerb;

    // Inventory
    SOT_InventoryItem inventory[SOT_INVENTORY_MAX];
    int inventoryCount;
    int selectedItem;           // Index into inventory (-1 = none)

    // Hover/interaction state
    int hoveredActorIndex;      // Actor index under cursor (-1 = none)
    char hoveredObjectName[128];// Name to display near cursor

    // Walk path
    float walkPath[SOT_WALK_PATH_MAX][2]; // Waypoints (x, y) in pixels
    int walkPathCount;
    int walkPathCurrent;        // Current waypoint index
    float walkSpeed;            // Pixels/sec

    // Walkable area polygon (loaded from scene/Tiled)
    float walkableArea[64][2];  // Polygon vertices
    int walkableVertexCount;
} SOT_Adventure;

// ---- API ----

void SOT_Adventure_Init(SOT_Adventure *adv);
void SOT_Adventure_Shutdown(SOT_Adventure *adv);

// Verb selection
void SOT_Adventure_SetVerb(SOT_Adventure *adv, SOT_Verb verb);
SOT_Verb SOT_Adventure_GetVerb(const SOT_Adventure *adv);
const char *SOT_Adventure_GetVerbName(SOT_Verb verb);

// Inventory management
bool SOT_Adventure_AddItem(SOT_Adventure *adv, const char *id, const char *name);
bool SOT_Adventure_RemoveItem(SOT_Adventure *adv, const char *id);
bool SOT_Adventure_HasItem(const SOT_Adventure *adv, const char *id);
void SOT_Adventure_SelectItem(SOT_Adventure *adv, int index);

// Per-frame update: hover detection, walk-to movement
void SOT_Adventure_Update(SOT_Adventure *adv,
                           struct SOT_Scene *scene,
                           struct SOT_PhysicsWorld *world,
                           float mouseGameX, float mouseGameY,
                           bool mouseClicked,
                           float deltaTime);

// Set walkable area from polygon vertices (pixels)
void SOT_Adventure_SetWalkableArea(SOT_Adventure *adv,
                                    const float (*vertices)[2], int count);

// Check if a point is inside the walkable area polygon
bool SOT_Adventure_IsPointWalkable(const SOT_Adventure *adv, float x, float y);

// Compute a walk path from start to end, staying inside walkable area.
// Returns number of waypoints (0 if no path). Simple direct path for now.
int SOT_Adventure_ComputeWalkPath(SOT_Adventure *adv,
                                   float startX, float startY,
                                   float endX, float endY);

#endif
