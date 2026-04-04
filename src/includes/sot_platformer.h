#ifndef SOT_PLATFORMER_H_
#define SOT_PLATFORMER_H_

#include <stdbool.h>
#include "sot_physics.h"

// Forward declarations
struct SOT_Actor;
struct SOT_Input;

// ---- Platformer controller parameters (tunable per-actor) ----

typedef struct SOT_PlatformerParams {
    // Horizontal movement
    float moveSpeed;            // Pixels/sec max horizontal speed
    float acceleration;         // Pixels/sec² ground acceleration
    float deceleration;         // Pixels/sec² ground deceleration (friction)
    float airAcceleration;      // Pixels/sec² aerial acceleration
    float airDeceleration;      // Pixels/sec² aerial deceleration

    // Jump
    float jumpForce;            // Impulse applied on jump (pixels/sec upward velocity)
    float jumpCutMultiplier;    // Velocity multiplied by this when jump released early (0.0-1.0)
    int maxAirJumps;            // Number of extra jumps in air (0 = single jump, 1 = double, etc.)
    float jumpBufferTime;       // Seconds to buffer a jump press before landing
    float coyoteTime;           // Seconds of grace period after leaving ground

    // Wall mechanics
    float wallSlideSpeed;       // Max fall speed when sliding on wall (pixels/sec)
    float wallJumpForceX;       // Horizontal impulse away from wall
    float wallJumpForceY;       // Vertical impulse on wall jump
    float wallClingTime;        // Seconds the player can cling before sliding (0 = no cling)

    // Dash
    float dashSpeed;            // Pixels/sec during dash
    float dashDuration;         // Seconds
    int maxAirDashes;           // Number of dashes allowed in air (0 = no air dash)
    float dashCooldown;         // Seconds between dashes

    // Detection
    float groundRayLength;      // Pixels below actor center to check for ground
    float wallRayLength;        // Pixels to the side to check for walls
    float groundRayOffsetX;     // Horizontal offset from center for ground rays (2 rays: left and right)
} SOT_PlatformerParams;

// ---- Platformer controller runtime state ----

typedef struct SOT_PlatformerController {
    SOT_PlatformerParams params;

    // Ground state
    bool grounded;
    bool wasGrounded;           // Grounded last frame
    float coyoteTimer;          // Counts down from coyoteTime after leaving ground
    float jumpBufferTimer;      // Counts down from jumpBufferTime after pressing jump

    // Jump state
    int airJumpsRemaining;
    bool jumpHeld;              // Is the jump button currently held?
    bool jumpedThisFrame;

    // Wall state
    bool touchingWallLeft;
    bool touchingWallRight;
    float wallClingTimer;

    // Dash state
    bool dashing;
    float dashTimer;
    float dashCooldownTimer;
    int airDashesRemaining;
    float dashDirX;             // Direction of current dash
    float dashDirY;

    // Input axes (set each frame before update)
    float inputX;               // -1..1 horizontal input
    float inputY;               // -1..1 vertical input
    bool jumpPressed;           // Just pressed this frame
    bool jumpReleased;          // Just released this frame
    bool dashPressed;           // Just pressed this frame
} SOT_PlatformerController;

// ---- API ----

// Initialize with default platformer parameters (snappy action-style)
void SOT_Platformer_InitDefault(SOT_PlatformerController *ctrl);

// Initialize with cinematic parameters (slower, deliberate movement)
void SOT_Platformer_InitCinematic(SOT_PlatformerController *ctrl);

// Set params from actor template properties (reads "move_speed", "jump_force", etc.)
void SOT_Platformer_LoadFromProperties(SOT_PlatformerController *ctrl,
                                        struct SOT_Actor *actor);

// Read input actions and store in controller (call before Update)
void SOT_Platformer_ReadInput(SOT_PlatformerController *ctrl, struct SOT_Input *input);

// Update the controller: detection, state machines, velocity application.
// Must be called every frame with the physics world and the actor's body.
void SOT_Platformer_Update(SOT_PlatformerController *ctrl,
                            SOT_PhysicsWorld *world,
                            struct SOT_Actor *actor,
                            float deltaTime);

#endif
