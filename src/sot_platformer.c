/*
 * SOT Platformer Controller
 *
 * Handles 2D platformer character mechanics: ground/wall detection via raycasts,
 * variable-height jump, wall-slide, wall-jump, dash, coyote time, input buffering.
 * All velocity changes go through Box2D (SetLinearVelocity / ApplyLinearImpulse).
 */

#include "sot_platformer.h"
#include "sot_actor.h"
#include "sot_input.h"
#include <math.h>

// ---- Default parameter presets ----

void SOT_Platformer_InitDefault(SOT_PlatformerController *ctrl)
{
    SDL_memset(ctrl, 0, sizeof(*ctrl));

    // Snappy action-platformer feel (Celeste/Mario style)
    ctrl->params = (SOT_PlatformerParams){
        .moveSpeed       = 120.0f,
        .acceleration    = 800.0f,
        .deceleration    = 600.0f,
        .airAcceleration = 600.0f,
        .airDeceleration = 300.0f,

        .jumpForce          = 200.0f,
        .jumpCutMultiplier  = 0.4f,
        .maxAirJumps        = 0,
        .jumpBufferTime     = 0.1f,
        .coyoteTime         = 0.1f,

        .wallSlideSpeed = 40.0f,
        .wallJumpForceX = 140.0f,
        .wallJumpForceY = 180.0f,
        .wallClingTime  = 0.0f,

        .dashSpeed     = 300.0f,
        .dashDuration  = 0.15f,
        .maxAirDashes  = 1,
        .dashCooldown  = 0.3f,

        .groundRayLength  = 3.0f,
        .wallRayLength    = 3.0f,
        .groundRayOffsetX = 4.0f,
    };
}

void SOT_Platformer_InitCinematic(SOT_PlatformerController *ctrl)
{
    SDL_memset(ctrl, 0, sizeof(*ctrl));

    // Deliberate cinematic feel (Flashback/Prince of Persia style)
    ctrl->params = (SOT_PlatformerParams){
        .moveSpeed       = 60.0f,
        .acceleration    = 300.0f,
        .deceleration    = 400.0f,
        .airAcceleration = 150.0f,
        .airDeceleration = 100.0f,

        .jumpForce          = 160.0f,
        .jumpCutMultiplier  = 0.7f,
        .maxAirJumps        = 0,
        .jumpBufferTime     = 0.08f,
        .coyoteTime         = 0.08f,

        .wallSlideSpeed = 50.0f,
        .wallJumpForceX = 0.0f,
        .wallJumpForceY = 0.0f,
        .wallClingTime  = 0.0f,

        .dashSpeed     = 0.0f,
        .dashDuration  = 0.0f,
        .maxAirDashes  = 0,
        .dashCooldown  = 0.0f,

        .groundRayLength  = 3.0f,
        .wallRayLength    = 3.0f,
        .groundRayOffsetX = 4.0f,
    };
}

void SOT_Platformer_LoadFromProperties(SOT_PlatformerController *ctrl,
                                        SOT_Actor *actor)
{
    SOT_PlatformerParams *p = &ctrl->params;
    float v;

    #define LOAD_FLOAT(name, field) \
        if (SOT_Actor_GetPropertyNumber(actor, name, &v)) p->field = v;

    LOAD_FLOAT("move_speed",        moveSpeed)
    LOAD_FLOAT("acceleration",      acceleration)
    LOAD_FLOAT("deceleration",      deceleration)
    LOAD_FLOAT("air_acceleration",  airAcceleration)
    LOAD_FLOAT("air_deceleration",  airDeceleration)
    LOAD_FLOAT("jump_force",        jumpForce)
    LOAD_FLOAT("jump_cut",          jumpCutMultiplier)
    LOAD_FLOAT("jump_buffer_time",  jumpBufferTime)
    LOAD_FLOAT("coyote_time",       coyoteTime)
    LOAD_FLOAT("wall_slide_speed",  wallSlideSpeed)
    LOAD_FLOAT("wall_jump_force_x", wallJumpForceX)
    LOAD_FLOAT("wall_jump_force_y", wallJumpForceY)
    LOAD_FLOAT("wall_cling_time",   wallClingTime)
    LOAD_FLOAT("dash_speed",        dashSpeed)
    LOAD_FLOAT("dash_duration",     dashDuration)
    LOAD_FLOAT("dash_cooldown",     dashCooldown)
    LOAD_FLOAT("ground_ray_length", groundRayLength)
    LOAD_FLOAT("wall_ray_length",   wallRayLength)

    if (SOT_Actor_GetPropertyNumber(actor, "max_air_jumps", &v))
        p->maxAirJumps = (int)v;
    if (SOT_Actor_GetPropertyNumber(actor, "max_air_dashes", &v))
        p->maxAirDashes = (int)v;

    #undef LOAD_FLOAT
}

// ---- Input ----

void SOT_Platformer_ReadInput(SOT_PlatformerController *ctrl, SOT_Input *input)
{
    // Horizontal axis: combine left/right actions
    float left  = SOT_Input_IsPressed(input, "move_left")  ? -1.0f : 0.0f;
    float right = SOT_Input_IsPressed(input, "move_right") ?  1.0f : 0.0f;
    ctrl->inputX = left + right;

    float up   = SOT_Input_IsPressed(input, "move_up")   ? -1.0f : 0.0f;
    float down = SOT_Input_IsPressed(input, "move_down") ?  1.0f : 0.0f;
    ctrl->inputY = up + down;

    ctrl->jumpPressed  = SOT_Input_IsJustPressed(input, "jump");
    ctrl->jumpReleased = SOT_Input_IsJustReleased(input, "jump");
    ctrl->jumpHeld     = SOT_Input_IsPressed(input, "jump");
    ctrl->dashPressed  = SOT_Input_IsJustPressed(input, "shoot");  // dash on shoot button
}

// ---- Helpers ----

static float MoveToward(float current, float target, float maxDelta)
{
    if (fabsf(target - current) <= maxDelta)
        return target;
    return current + (target > current ? maxDelta : -maxDelta);
}

// ---- Core update ----

void SOT_Platformer_Update(SOT_PlatformerController *ctrl,
                            SOT_PhysicsWorld *world,
                            SOT_Actor *actor,
                            float dt)
{
    if (!b2Body_IsValid(actor->bodyId)) return;

    SOT_PlatformerParams *p = &ctrl->params;
    ctrl->jumpedThisFrame = false;

    // Get current position and velocity in pixels
    b2Vec2 bpos = b2Body_GetPosition(actor->bodyId);
    float px, py;
    SOT_MetersToPixels(bpos, &px, &py);

    b2Vec2 bvel = b2Body_GetLinearVelocity(actor->bodyId);
    float vx = bvel.x * SOT_PIXELS_PER_METER;
    float vy = bvel.y * SOT_PIXELS_PER_METER;

    // ---- Ground detection (two raycasts from feet) ----
    ctrl->wasGrounded = ctrl->grounded;
    {
        float rayY1 = py;
        float rayY2 = py + p->groundRayLength;
        uint64_t mask = SOT_CAT_TERRAIN;

        float frac;
        b2Vec2 normal;
        b2BodyId hitL = SOT_Physics_Raycast(world, px - p->groundRayOffsetX, rayY1,
                                             px - p->groundRayOffsetX, rayY2,
                                             mask, &frac, &normal);
        b2BodyId hitR = SOT_Physics_Raycast(world, px + p->groundRayOffsetX, rayY1,
                                             px + p->groundRayOffsetX, rayY2,
                                             mask, &frac, &normal);
        ctrl->grounded = b2Body_IsValid(hitL) || b2Body_IsValid(hitR);
    }

    // ---- Wall detection (side raycasts) ----
    {
        float frac;
        b2Vec2 normal;
        uint64_t mask = SOT_CAT_TERRAIN;

        b2BodyId hitL = SOT_Physics_Raycast(world, px, py,
                                             px - p->wallRayLength, py,
                                             mask, &frac, &normal);
        b2BodyId hitR = SOT_Physics_Raycast(world, px, py,
                                             px + p->wallRayLength, py,
                                             mask, &frac, &normal);
        ctrl->touchingWallLeft  = b2Body_IsValid(hitL);
        ctrl->touchingWallRight = b2Body_IsValid(hitR);
    }

    // ---- Coyote time ----
    if (ctrl->grounded) {
        ctrl->coyoteTimer = p->coyoteTime;
        ctrl->airJumpsRemaining = p->maxAirJumps;
        ctrl->airDashesRemaining = p->maxAirDashes;
    } else {
        ctrl->coyoteTimer -= dt;
    }

    bool canCoyoteJump = (!ctrl->grounded && ctrl->coyoteTimer > 0.0f);

    // ---- Jump buffer ----
    if (ctrl->jumpPressed) {
        ctrl->jumpBufferTimer = p->jumpBufferTime;
    } else {
        ctrl->jumpBufferTimer -= dt;
    }

    bool wantsJump = (ctrl->jumpBufferTimer > 0.0f);

    // ---- Dash ----
    if (ctrl->dashCooldownTimer > 0.0f)
        ctrl->dashCooldownTimer -= dt;

    if (ctrl->dashing) {
        ctrl->dashTimer -= dt;
        if (ctrl->dashTimer <= 0.0f) {
            ctrl->dashing = false;
            // Restore gravity
            b2Body_SetGravityScale(actor->bodyId, 1.0f);
        } else {
            // During dash: override velocity
            vx = ctrl->dashDirX * p->dashSpeed;
            vy = ctrl->dashDirY * p->dashSpeed;
            goto apply_velocity;
        }
    }

    if (ctrl->dashPressed && p->dashSpeed > 0.0f && ctrl->dashCooldownTimer <= 0.0f) {
        bool canDash = ctrl->grounded || ctrl->airDashesRemaining > 0;
        if (canDash) {
            ctrl->dashing = true;
            ctrl->dashTimer = p->dashDuration;
            ctrl->dashCooldownTimer = p->dashCooldown;
            if (!ctrl->grounded) ctrl->airDashesRemaining--;

            // Dash direction: input direction, or facing direction if no input
            ctrl->dashDirX = ctrl->inputX;
            ctrl->dashDirY = 0.0f;
            if (ctrl->dashDirX == 0.0f && ctrl->dashDirY == 0.0f)
                ctrl->dashDirX = 1.0f; // Default: right

            // Zero gravity during dash
            b2Body_SetGravityScale(actor->bodyId, 0.0f);
            vy = 0.0f;
            vx = ctrl->dashDirX * p->dashSpeed;
            goto apply_velocity;
        }
    }

    // ---- Wall slide ----
    bool wallSliding = false;
    if (!ctrl->grounded && (ctrl->touchingWallLeft || ctrl->touchingWallRight)) {
        // Only wall-slide if pressing toward the wall and falling
        bool pressingToWall = (ctrl->touchingWallLeft && ctrl->inputX < 0.0f) ||
                              (ctrl->touchingWallRight && ctrl->inputX > 0.0f);
        if (pressingToWall && vy > 0.0f) {
            wallSliding = true;
            if (vy > p->wallSlideSpeed)
                vy = p->wallSlideSpeed;
        }
    }

    // ---- Jump ----
    if (wantsJump) {
        bool canJump = ctrl->grounded || canCoyoteJump;

        // Wall jump
        if (!canJump && p->wallJumpForceX > 0.0f &&
            (ctrl->touchingWallLeft || ctrl->touchingWallRight)) {
            canJump = true;
            float wallDir = ctrl->touchingWallLeft ? 1.0f : -1.0f;
            vx = wallDir * p->wallJumpForceX;
            vy = -p->wallJumpForceY;
            ctrl->jumpBufferTimer = 0.0f;
            ctrl->coyoteTimer = 0.0f;
            ctrl->jumpedThisFrame = true;
            goto apply_velocity;
        }

        // Air jump
        if (!canJump && ctrl->airJumpsRemaining > 0) {
            canJump = true;
            ctrl->airJumpsRemaining--;
        }

        if (canJump) {
            vy = -p->jumpForce;
            ctrl->jumpBufferTimer = 0.0f;
            ctrl->coyoteTimer = 0.0f;
            ctrl->jumpedThisFrame = true;
        }
    }

    // ---- Variable jump height (cut velocity on release) ----
    if (ctrl->jumpReleased && vy < 0.0f) {
        vy *= p->jumpCutMultiplier;
    }

    // ---- Horizontal movement ----
    {
        float targetVX = ctrl->inputX * p->moveSpeed;
        float accel, decel;

        if (ctrl->grounded) {
            accel = p->acceleration;
            decel = p->deceleration;
        } else {
            accel = p->airAcceleration;
            decel = p->airDeceleration;
        }

        if (fabsf(ctrl->inputX) > 0.01f) {
            vx = MoveToward(vx, targetVX, accel * dt);
        } else {
            vx = MoveToward(vx, 0.0f, decel * dt);
        }
    }

apply_velocity:
    {
        b2Vec2 newVel = {
            vx * SOT_METERS_PER_PIXEL,
            vy * SOT_METERS_PER_PIXEL,
        };
        b2Body_SetLinearVelocity(actor->bodyId, newVel);
    }
}
