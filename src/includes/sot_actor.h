#ifndef SOT_ACTOR_H_
#define SOT_ACTOR_H_

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "cglm.h"
#include "cute_c2.h"
#include "appstate.h"
#include "sot_physics.h"
#include "sot_animation.h"
#include "sot_collider.h"
#include "sot_math_interop.h"

// ---- Property bag ----

#define SOT_ACTOR_MAX_PROPERTIES 32
#define SOT_ACTOR_MAX_TAGS       8
#define SOT_ACTOR_MAX_TAG_LEN    32

typedef enum {
    SOT_PROP_NONE = 0,
    SOT_PROP_NUMBER,
    SOT_PROP_STRING,
    SOT_PROP_BOOL,
} SOT_PropertyType;

typedef struct SOT_Property {
    char key[64];
    SOT_PropertyType type;
    union {
        float number;
        char string[128];
        bool boolean;
    } value;
} SOT_Property;

// ---- Lifecycle callbacks (C function pointers, will map to Lua later) ----

struct SOT_Actor;

typedef void (*SOT_ActorCallback_Create)(struct SOT_Actor *self);
typedef void (*SOT_ActorCallback_Update)(struct SOT_Actor *self, float deltaTime);
typedef void (*SOT_ActorCallback_Destroy)(struct SOT_Actor *self);
typedef void (*SOT_ActorCallback_Collision)(struct SOT_Actor *self, struct SOT_Actor *other);
typedef void (*SOT_ActorCallback_TriggerEnter)(struct SOT_Actor *self, struct SOT_Actor *other);
typedef void (*SOT_ActorCallback_TriggerExit)(struct SOT_Actor *self, struct SOT_Actor *other);

typedef struct SOT_ActorCallbacks {
    SOT_ActorCallback_Create      on_create;
    SOT_ActorCallback_Update      on_update;
    SOT_ActorCallback_Destroy     on_destroy;
    SOT_ActorCallback_Collision   on_collision;
    SOT_ActorCallback_TriggerEnter on_trigger_enter;
    SOT_ActorCallback_TriggerExit  on_trigger_exit;
} SOT_ActorCallbacks;

// ---- State machine ----

#define SOT_ACTOR_MAX_STATES 32

typedef struct SOT_StateMapping {
    char stateName[64];       // Gameplay state (e.g. "idle", "walk_right")
    char animationName[64];   // Animation sequence name (e.g. "idle_R", "walk_R")
} SOT_StateMapping;

// ---- Actor body type (maps to Box2D) ----

typedef enum {
    SOT_BODY_NONE = 0,
    SOT_BODY_STATIC,
    SOT_BODY_DYNAMIC,
    SOT_BODY_KINEMATIC,
} SOT_BodyType;

// ---- Actor template (loaded from JSON) ----

typedef struct SOT_ActorTemplate {
    char name[64];
    char animationFile[128];
    SOT_BodyType bodyType;
    bool fixedRotation;
    float gravityScale;

    // Collider shape definition
    char colliderType[16];   // "box", "circle", "none"
    float colliderHalfW;
    float colliderHalfH;
    float colliderRadius;
    float colliderOffsetX;
    float colliderOffsetY;
    bool  colliderIsSensor;
    uint64_t categoryBits;
    uint64_t maskBits;

    // Default properties
    SOT_Property properties[SOT_ACTOR_MAX_PROPERTIES];
    int propertyCount;

    // Tags
    char tags[SOT_ACTOR_MAX_TAGS][SOT_ACTOR_MAX_TAG_LEN];
    int tagCount;

    // Script file (for future Lua integration)
    char scriptFile[128];

    // State machine (state name → animation sequence name)
    SOT_StateMapping states[SOT_ACTOR_MAX_STATES];
    int stateCount;
} SOT_ActorTemplate;

// ---- Direction enum (legacy, kept for animation state) ----

typedef enum {
    IDLE,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN,
    FALL,
    JUMP
} Direction;

// ---- Actor ----

typedef struct SOT_Actor {
    int actorID;
    char actorName[64];
    char templateName[64];      // Name of the template this actor was created from (empty if manual)
    bool enabled;               // If false, actor is not updated or rendered
    SOT_Transform transform;

    // Box2D physics body (b2_nullBodyId if no physics)
    b2BodyId bodyId;

    // Animation
    SOT_AnimationInfo *animationInfos[16];
    int animationInfoCount;
    SOT_Animation animations[256];
    int animationsCount;
    int currentAnimation;

    // Legacy collider (cute_c2) -- kept for debug rendering until fully migrated
    sot_collider_t collider;

    // Property bag
    SOT_Property properties[SOT_ACTOR_MAX_PROPERTIES];
    int propertyCount;

    // Tags
    char tags[SOT_ACTOR_MAX_TAGS][SOT_ACTOR_MAX_TAG_LEN];
    int tagCount;

    // Lifecycle callbacks
    SOT_ActorCallbacks callbacks;

    // State machine
    char currentState[64];
    SOT_StateMapping states[SOT_ACTOR_MAX_STATES];
    int stateCount;
} SOT_Actor;

// ---- Template loading/saving ----
SOT_ActorTemplate *SOT_LoadActorTemplate(const char *templateName);
void SOT_FreeActorTemplate(SOT_ActorTemplate *tmpl);
bool SOT_SaveActorTemplate(const SOT_ActorTemplate *tmpl, const char *templateName);

// ---- State machine ----
bool SOT_Actor_SetState(SOT_Actor *actor, const char *stateName);

// ---- Actor creation ----
SOT_Actor SOT_CreateActor(AppState *appState, char *name, vec2 pos, char *animationsFile);
SOT_Actor SOT_CreateActorFromTemplate(AppState *appState, SOT_ActorTemplate *tmpl, vec2 pos,
                                       const char *instanceName);

void SOT_ActorBindAnimations(SOT_Actor *actor, SOT_AnimationInfo *animationInfo);

// ---- Property bag API ----
void SOT_Actor_SetPropertyNumber(SOT_Actor *actor, const char *key, float value);
void SOT_Actor_SetPropertyString(SOT_Actor *actor, const char *key, const char *value);
void SOT_Actor_SetPropertyBool(SOT_Actor *actor, const char *key, bool value);
bool SOT_Actor_GetPropertyNumber(const SOT_Actor *actor, const char *key, float *outValue);
bool SOT_Actor_GetPropertyString(const SOT_Actor *actor, const char *key, const char **outValue);
bool SOT_Actor_GetPropertyBool(const SOT_Actor *actor, const char *key, bool *outValue);

// ---- Tag API ----
void SOT_Actor_AddTag(SOT_Actor *actor, const char *tag);
bool SOT_Actor_HasTag(const SOT_Actor *actor, const char *tag);

// ---- Enable/disable ----
void SOT_Actor_SetEnabled(SOT_Actor *actor, bool enabled);

// ---- Physics body setup (from template) ----
struct SOT_PhysicsWorld;
void SOT_Actor_CreatePhysicsBody(SOT_Actor *actor, struct SOT_PhysicsWorld *world, SOT_ActorTemplate *tmpl);

// ---- Transform and physics ----
void SetPosition(SOT_Actor *actor, vec2 pos);
void SetVelocity(SOT_Actor *actor, vec2 vel);
void SetCollider(SOT_Actor *actor, sot_collider_t collider);
void UpdateCollider(SOT_Actor *actor, vec2 deltaPos);

// ---- Lifecycle ----
void MoveActor(SOT_Actor *actor, SDL_Event *event);
void Hit(const AppState *as, SOT_Actor *actor);
void UpdateActor(const AppState *as, SOT_Actor *actor, float deltaTime);
void SetRenderPosition(SOT_Actor *actor);
void RenderActor(const AppState *as, SOT_Actor *actor);
void DestroyActor(SOT_Actor *actor);

#endif
