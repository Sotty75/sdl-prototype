# SOT Engine -- Requirements Document

## 0. Preamble

### 0.1 Project Identity

**Engine name prefix:** `SOT_`
**Language:** C11
**Graphics backend:** SDL3 GPU API (Vulkan via SPIR-V shaders)
**Toolchain:** MinGW-w64 / MSYS2 (UCRT64), CMake 3.21+, Ninja
**Target platform:** Windows (with potential Linux portability via SDL3)

### 0.2 Target Game Types

The engine shall support development of three distinct 2D game genres. Every subsystem requirement is evaluated against these three archetypes:

| Code | Genre | Reference Titles |
|------|-------|-----------------|
| **SHMUP** | Horizontal/vertical shoot-em-up | R-Type, Gradius, Axelay |
| **PLAT** | 2D platformer (action and cinematic) | Celeste, Super Mario, Mega Man, Flashback, Prince of Persia |
| **ADV** | Point-and-click adventure | Monkey Island, Day of the Tentacle, Full Throttle |

### 0.3 Priority Tiers

| Tier | Meaning |
|------|---------|
| **T1** | Must-have for any of the three game types to function at all |
| **T2** | Required by one or two specific game types; engine is usable without it but a target game is not |
| **T3** | Quality-of-life, polish, or tooling improvement; deferrable |

### 0.4 Existing Codebase Baseline

The following subsystems already exist in some form and shall be enhanced, not replaced:

- **Rendering:** Multi-pipeline GPU renderer (`SOT_RP_TILEMAP`, `SOT_RP_SPRITE`, `SOT_RP_OVERLAY`, `SOT_RP_DEBUG`) using instanced/SSBO-based draw calls. Nearest-neighbor sampler. Sprite pipeline recently completed.
- **Tilemap:** Single draw call tilemap renderer. Tiled JSON loader via `cute_tiled.h`. Collision shapes extracted from Tiled object layers.
- **Animation:** JSON-defined spritesheet atlas system (`SOT_AnimationInfo` / `SOT_Animation` / `SOT_AnimationSequence`). Per-frame UV coordinates. Playback timing.
- **Collision:** `cute_c2.h` -- AABB, circle, capsule, polygon shapes. Static and dynamic collider linked lists. Manifold-based collision response.
- **Camera:** Perspective and orthographic projection. View/projection matrix pipeline via cglm. Pan-based camera update.
- **Scene:** JSON scene descriptor referencing a Tiled map and up to 16 spritesheets. Single scene loading with actor initialization. Fixed-size actor array (2000 max).
- **Actor:** Monolithic struct with transform, physics, animation (up to 256 instances from 16 files), and collider. Kinematic body type. Gamepad-driven movement.
- **Input:** Keyboard scancode handling and gamepad axis/button support via SDL3 event loop.

### 0.5 Established Conventions

- Structs: `SOT_PascalCase` (public types) or `sot_snake_case_t` (internal/lower-level types)
- Functions: `SOT_ModuleVerb()` for public API, `VerbNoun()` for local helpers
- Global state: `AppState` singleton passed as first argument
- Memory: `SDL_malloc`/`SDL_calloc`/`SDL_free` throughout (not stdlib)
- JSON parsing: cJSON library
- Math: cglm with `CGLM_FORCE_LEFT_HANDED` and `CGLM_FORCE_DEPTH_ZERO_TO_ONE`
- Shaders: GLSL compiled to SPIR-V via `glslc`
- Asset resolution: `SOT_Paths` global struct resolving relative to the executable directory

---

## 1. Display System (`sot_display`)

### 1.1 Purpose

Manages the virtual framebuffer that gives the engine its retro aesthetic. All game rendering targets a low internal resolution; this module handles upscaling that framebuffer to the actual window/display resolution. Critical for all three game types -- the pixel-art look is the engine's visual identity.

### 1.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| DSP-01 | Virtual framebuffer | T1 | All game rendering targets an off-screen render texture at configurable internal resolution (default 320x240). The final presentation pass scales this texture to the window. |
| DSP-02 | Integer scaling | T1 | The virtual framebuffer is integer-scaled to the largest multiple that fits the window, with letterboxing (black bars) filling the remainder. No fractional scaling; pixels remain sharp. |
| DSP-03 | Configurable internal resolution | T1 | The internal resolution (width, height) shall be settable per-project. Common presets: 256x224 (SNES), 320x200 (DOS), 320x240 (common retro), 384x216 (16:9 retro). |
| DSP-04 | Window management | T1 | Support for windowed mode, borderless fullscreen, and exclusive fullscreen. Toggling between modes shall not lose GPU state. |
| DSP-05 | Aspect ratio preservation | T1 | When the window aspect ratio does not match the internal resolution, letterboxing shall be applied. The game image is never stretched. |
| DSP-06 | CRT/scanline filter | T3 | Optional post-process shader pass simulating CRT scanlines, slight curvature, or color bleed. Applied during the upscale blit, not to game logic. |

### 1.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| DSP-07 | Screen shake | T2 | SHMUP, PLAT | Offset the virtual framebuffer by a decaying random displacement. Used for explosions (SHMUP) and heavy landings (PLAT). |

### 1.4 Data Format

- Internal resolution and display preferences stored in a project-level JSON config file (`project.json` or equivalent).
- No per-scene display data; display settings are global.

### 1.5 Integration Points

- **Rendering System:** All rendering pipelines render to the virtual framebuffer texture, not directly to the swapchain. The final presentation pass (a fullscreen quad with the framebuffer texture) is owned by the display system.
- **Editor (ImGui):** When the editor is active, the virtual framebuffer is rendered into an ImGui image panel rather than fullscreen. The editor UI itself renders at native resolution.

---

## 2. Rendering System (`sot_gpu_pipeline` -- extend existing)

### 2.1 Purpose

The low-level rendering subsystem that draws all visible content. Extends the existing multi-pipeline architecture to support layered compositing, visual effects, and text. Every game type depends on this.

### 2.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| RND-01 | Layer system | T1 | Support an ordered stack of render layers. Each layer has a depth/order index. Content within a layer is sorted by z-order. Layers compose back-to-front into the virtual framebuffer. |
| RND-02 | Parallax scrolling | T1 | Background layers scroll at configurable speed ratios relative to the main camera. Layers may tile/repeat horizontally and/or vertically. |
| RND-03 | Sprite batching (existing, enhance) | T1 | The existing SSBO-based instanced sprite pipeline shall support sorting by texture atlas and z-order within a layer. Per-sprite alpha, horizontal/vertical flip, and tint color. |
| RND-04 | Tilemap rendering (existing, enhance) | T1 | The existing single-draw-call tilemap pipeline shall support multiple tilemap layers per scene (background, midground, foreground). Each layer is a separate tilemap. |
| RND-05 | Alpha blending | T1 | Full alpha blending support for sprites, UI elements, and particle effects. Pre-multiplied alpha pipeline configuration. |
| RND-06 | Bitmap font rendering | T1 | Render text from a bitmap font atlas (monospaced and proportional). Support for text alignment, word wrapping at a given width, and basic formatting (color per character). |
| RND-07 | Screen transitions | T2 | Built-in transition effects between scenes: fade to/from black, crossfade, horizontal/vertical wipe, iris (circle) open/close, dissolve (random pixel dither). Implemented as post-process passes on the virtual framebuffer. |
| RND-08 | Palette effects | T2 | Global screen effects: fade to color (arbitrary, not just black), flash white (hit feedback), color wash/tint. Implemented as a uniform color multiply/add in the final presentation shader or per-layer. |
| RND-09 | Particle system | T2 | Lightweight GPU-friendly particle emitter. Each particle: position, velocity, lifetime, color, size, texture region. Configurable emission rate, spread, gravity. Rendered via the sprite pipeline or a dedicated particle SSBO. |
| RND-10 | Sprite flip | T1 | Per-sprite horizontal and vertical flip flags, applied in the vertex shader via UV mirroring. |
| RND-11 | Sprite tint/color modulation | T2 | Per-sprite RGBA color multiplier in the sprite instance data, applied in the fragment shader. |

### 2.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| RND-12 | Multi-layer parallax backgrounds | T1 | SHMUP | At least 4 parallax layers scrolling at independent speeds. Continuous auto-scroll for the main game layer. |
| RND-13 | Foreground overlay layer | T2 | PLAT | A layer rendered in front of actors (e.g., foreground foliage, cage bars) for depth illusion. |
| RND-14 | Full-screen animated backgrounds | T2 | ADV | Support for a single large image (or animated sequence) as the room background, not necessarily tile-based. |
| RND-15 | Walk-behind regions | T2 | ADV | Mask or depth layer that makes the player character walk behind parts of the background art. |

### 2.4 Data Format

- **Layer definitions:** Part of the scene JSON descriptor. Each layer specifies: type (tilemap, image, parallax), asset reference, scroll multiplier, z-order.
- **Bitmap fonts:** JSON descriptor mapping character codes to atlas regions, plus kerning data. Font atlas as PNG.
- **Particle emitter definitions:** JSON files describing emitter properties (template). Runtime instantiation from templates.
- **Transition types:** Enum in code; parameterized by duration and color at call site.

### 2.5 Integration Points

- **Display System:** Rendering writes to the virtual framebuffer owned by DSP.
- **Scene/World System:** Scene descriptor defines which layers exist, their order, and parallax rates.
- **Animation System:** Sprites rendered by the rendering system consume animation frame data.
- **Camera System:** Camera position drives parallax offset calculation per layer.
- **UI System:** UI elements rendered on a dedicated top layer with no parallax.

---

## 3. Audio System (`sot_audio` -- new)

### 3.1 Purpose

Provides music playback and sound effect mixing. No audio capability exists currently. Critical for all three game types -- music sets mood, sound effects provide feedback.

### 3.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| AUD-01 | Music playback | T1 | Play streamed music from OGG Vorbis files. Support play, pause, stop, and seek operations. One music track active at a time. |
| AUD-02 | Tracker music playback | T2 | Play tracker module formats (.mod, .xm, .it) for authentic retro feel. Requires integration of a playback library (e.g., libxmp, libopenmpt, or SDL3_mixer). |
| AUD-03 | Sound effect playback | T1 | Play short sound effects from WAV or OGG files. Support concurrent playback of multiple effects. |
| AUD-04 | Volume control | T1 | Independent volume controls for: master, music, and sound effects. Volumes are normalized floats (0.0 to 1.0). |
| AUD-05 | Music crossfade | T2 | Transition between two music tracks with configurable crossfade duration. Used when moving between areas/rooms. |
| AUD-06 | Sound effect pooling | T2 | Pre-load frequently used sound effects into a pool to avoid per-play file I/O. Pool populated from a manifest or lazily on first use. |
| AUD-07 | Channel limit | T1 | Configurable maximum number of simultaneous sound effect channels (default 16). When the limit is reached, the oldest or lowest-priority sound is evicted. |

### 3.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| AUD-08 | Rapid-fire SFX deduplication | T2 | SHMUP | When the same sound effect is triggered many times per frame (e.g., bullet fire), limit to N concurrent instances and slightly vary pitch to avoid phasing. |
| AUD-09 | Ambient/looping SFX | T2 | ADV, PLAT | Support for looping ambient sounds attached to scene regions (e.g., waterfall, machine hum). Start/stop when camera enters/leaves region. |
| AUD-10 | Dialog audio cues | T3 | ADV | Support for per-dialog-line voice clips or typewriter-tick sounds during text display. |

### 3.4 Data Format

- **Music files:** `.ogg` in `assets/music/`. Tracker files `.mod`, `.xm`, `.it` in the same directory.
- **Sound effects:** `.wav` or `.ogg` in `assets/sfx/`.
- **Audio manifest (optional):** JSON file listing pre-loaded sound pools and their properties (volume, max instances, priority).

### 3.5 Integration Points

- **Scripting (Lua):** Lua API for playing music, triggering sounds, and controlling volume. All audio triggers from gameplay originate in scripts.
- **Animation System:** Frame events (ANM-03) can trigger sound effects on specific animation frames.
- **Scene System:** Scene transitions trigger music crossfades. Scene descriptors may reference a default music track and ambient sound set.

---

## 4. Input System (`sot_input` -- new, replacing inline event handling)

### 4.1 Purpose

Abstracts physical input devices into logical game actions. The existing codebase handles raw SDL events inline in `SDL_AppEvent` and `MoveActor`. This module replaces that with a proper action-mapping layer. Critical for all three game types, each of which has fundamentally different input paradigms.

### 4.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| INP-01 | Action mapping | T1 | Define named actions (e.g., `"jump"`, `"shoot"`, `"interact"`) and bind them to one or more physical inputs (keyboard scancodes, gamepad buttons/axes, mouse buttons). |
| INP-02 | Keyboard input | T1 | Full keyboard state tracking via SDL3. Detect pressed, just-pressed (this frame), and just-released (this frame). |
| INP-03 | Gamepad input (existing, enhance) | T1 | Extend existing gamepad support with action mapping, configurable deadzone per axis, and support for multiple gamepads. |
| INP-04 | Mouse input | T1 | Track mouse position (in virtual framebuffer coordinates, not window coordinates), button state, and cursor visibility. Convert window-space mouse to game-space via the inverse of the display scaling. |
| INP-05 | Action query API | T1 | Functions to query action state: `SOT_Input_IsActionPressed("jump")`, `SOT_Input_IsActionJustPressed("jump")`, `SOT_Input_IsActionJustReleased("jump")`, `SOT_Input_GetActionAxis("move_x")`. |
| INP-06 | Input rebinding | T2 | Runtime rebinding of actions to different physical inputs. Persisted to a JSON config file. |
| INP-07 | Input context stacking | T2 | Multiple input contexts (e.g., "gameplay", "menu", "dialog", "editor") with independent action maps. Push/pop contexts onto a stack; only the top context receives input. |

### 4.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| INP-08 | Input buffering | T1 | PLAT | Buffer recent action presses for a configurable window (e.g., 100ms). Allows jump-buffering (pressing jump before landing), dash-buffering, and wall-jump-buffering. Critical for tight action-platformer feel (Celeste, Mario). Also valuable for cinematic platformers to forgive timing on ledge grabs. |
| INP-09 | Coyote time | T1 | PLAT | After walking off a ledge, the "grounded" state persists for a configurable grace period (e.g., 80-120ms), allowing a late jump input. Essential for fair-feeling platformer controls across all platformer sub-genres. (Physics-adjacent but driven by the input system's timing.) |
| INP-10 | Mouse cursor and hotspot detection | T1 | ADV | Cursor rendered as a sprite at the mouse position. When hovering over interactive objects, the cursor changes appearance and the object name is displayed. |
| INP-11 | Auto-fire / hold | T2 | SHMUP | Actions can be configured as "auto-repeat while held" with a configurable repeat rate. |

### 4.4 Data Format

- **Action maps:** JSON file defining action names, default bindings, and properties (buffered, auto-repeat, axis vs. button).
- **User overrides:** Separate JSON file for player-customized bindings, merged on top of defaults at load.

### 4.5 Integration Points

- **Scripting (Lua):** Lua queries actions by name, never raw keys. Lua can push/pop input contexts.
- **UI System:** Menu navigation uses a dedicated input context.
- **Editor:** Editor mode uses its own input context that does not conflict with game actions.
- **Actor/Entity System:** Actor update functions query actions rather than processing raw events.

---

## 5. Scene/World System (`sot_scene` -- extend existing)

### 5.1 Purpose

Manages the spatial organization of the game world: what rooms/levels exist, what is in them, how the camera behaves, and how transitions between scenes work. The existing system loads a single scene from a JSON descriptor. This shall be extended to a multi-scene manager with transitions and multiple scrolling modes.

### 5.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| SCN-01 | Scene manager | T1 | Maintain a registry of available scenes. Load/unload scenes by name or ID. Exactly one scene is active at a time (with brief overlap during transitions). |
| SCN-02 | Scene descriptor (existing, enhance) | T1 | Extend the existing JSON scene descriptor to include: layer definitions (parallax, tilemap, image), actor spawn list (with position, type, and properties), camera configuration, default music track, and entry/exit points. |
| SCN-03 | Scene transitions | T1 | Switch between scenes with a visual transition effect (see RND-07). The old scene is rendered to a snapshot texture, the new scene loads, and the transition effect composites between them. |
| SCN-04 | Actor spawn points | T1 | Define named spawn points in the scene descriptor or Tiled map (as object layer markers). Actors reference spawn points by name for initial placement. |
| SCN-05 | Scene save/load (serialization) | T2 | Serialize and deserialize the mutable state of a scene (actor positions, inventory state, puzzle flags) to/from a save file. Enables save games. |

### 5.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| SCN-06 | Auto-scrolling | T1 | SHMUP | Camera scrolls automatically at a configurable speed along a defined axis (horizontal or vertical). Actors are constrained to the visible area. Scroll speed can change mid-level (speed ramps, boss pauses). |
| SCN-07 | Camera follow with dead zone | T1 | PLAT | Camera tracks the player with a configurable dead zone (a rectangle in screen space where the player can move without causing camera motion). Smooth easing when the player exits the dead zone. Camera clamped to level bounds. |
| SCN-08 | Room-snap camera | T1 | ADV | Camera is locked to discrete room boundaries. Entering a room transition trigger snaps (or slides) the camera to the new room. No continuous scrolling. |
| SCN-09 | Level segments / sections | T2 | SHMUP | A level is composed of sequential sections with different scroll speeds, enemy wave triggers, and mid-boss encounters. Section boundaries defined in scene data. |
| SCN-10 | Room graph / connections | T2 | ADV | Rooms are connected by named exits. Scene descriptor defines which exit leads to which room and at which entry point. |
| SCN-11 | Checkpoints | T2 | PLAT | Named checkpoint positions within a scene. On player death, respawn at the last activated checkpoint rather than the scene start. |

### 5.4 Data Format

- **Scene descriptor:** JSON, extending the existing format. Path: `assets/scenes/<scene_name>.json`.
- **Tiled map data:** JSON exported from Tiled editor. Object layers used for spawn points, triggers, camera bounds, room exits.
- **Save files:** JSON serialization of scene state. Path: `saves/<slot>.json`.

### 5.5 Integration Points

- **Rendering System:** Scene provides layer configuration and camera matrices to the renderer.
- **Entity/Actor System:** Scene owns the actor list and drives actor lifecycle.
- **Audio System:** Scene transitions trigger music changes.
- **Scripting (Lua):** Scene lifecycle callbacks (`on_scene_enter`, `on_scene_exit`). Lua can trigger scene transitions.
- **Editor:** Scene descriptor is the primary document edited in the editor.

---

## 6. Entity/Actor System (`sot_actor` -- refactor existing)

### 6.1 Purpose

Manages all dynamic objects in the game world: players, enemies, NPCs, projectiles, collectibles, triggers, and interactive hotspots. The existing `SOT_Actor` is a monolithic struct with a fixed set of capabilities. This section defines requirements for a more flexible system while remaining practical for a solo developer.

### 6.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| ENT-01 | Actor archetype / template system | T1 | Define actor types in JSON files (e.g., `enemy_drone.json`, `npc_bartender.json`). Templates specify: default animation file, collider shape/size, physics body type, script file, and custom properties. Instantiation creates an actor from a template with optional overrides. |
| ENT-02 | Actor lifecycle callbacks | T1 | Each actor has lifecycle hooks invoked by the engine: `on_create`, `on_update(dt)`, `on_destroy`, `on_collision(other, manifold)`. These map to Lua script functions. |
| ENT-03 | Actor properties (key-value) | T1 | Each actor carries a string-keyed property bag (string, number, or bool values). Set in the template, overridable per-instance in the scene descriptor, readable/writable from Lua. Used for game-specific data (health, score value, dialog ID, etc.). |
| ENT-04 | Actor enable/disable | T1 | Actors can be disabled (hidden and not updated) without destroying them. Used for object pooling, toggling visibility, and deferred activation. |
| ENT-05 | Actor tagging | T2 | Assign string tags to actors (e.g., "enemy", "collectible", "npc"). Query actors by tag. Useful for Lua gameplay logic ("destroy all enemies"). |
| ENT-06 | Actor parent-child hierarchy | T2 | Attach actors to a parent. Child transforms are relative to parent. Used for: weapon attached to ship (SHMUP), held item (PLAT), character with separate arm/head sprites (ADV). |
| ENT-07 | Actor pooling | T2 | Pre-allocate a pool of actors for frequently spawned/destroyed types (bullets, particles, collectibles). Recycle instead of create/destroy to avoid allocation churn. |

### 6.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| ENT-08 | Bullet actor type | T1 | SHMUP | Lightweight actor variant optimized for high volume: minimal properties, simple collider, no Lua per-update (pattern logic in spawner). Pool-allocated. |
| ENT-09 | NPC actor type | T1 | ADV | Actor with: interaction hotspot (click target), dialog tree reference, idle animation, walk path capability. |
| ENT-10 | Platformer character controller | T1 | PLAT | Actor with configurable platformer mechanics: grounded detection, wall-touch detection, variable jump height, wall-slide/wall-jump, dash (ground + air), double/multi-jump (configurable count), ledge grab zones. All exposed as properties/flags consumed by the physics step and Lua controller script. Supports both snappy action-platformer feel (Celeste/Mario) and deliberate cinematic feel (Flashback/Prince of Persia) via tunable parameters. |

### 6.4 Data Format

- **Actor templates:** JSON files in `assets/actors/<type>.json`.
- **Instance overrides:** Defined inline in the scene descriptor JSON, or as Tiled map object properties.

### 6.5 Integration Points

- **Scripting (Lua):** Actor lifecycle hooks are Lua functions. Lua creates, queries, and destroys actors.
- **Animation System:** Actor references animations by name from its template. Current animation driven by Lua or engine state machine.
- **Collision System:** Actor collider shape and layer/mask defined in template. Collision events dispatched to Lua callbacks.
- **Rendering System:** Actor sprite instance data (position, frame, atlas) pushed to GPU each frame.
- **Scene System:** Scene owns and iterates the actor list.

### 6.6 Architectural Decision: ECS vs. Enhanced Monolithic Actor

Given the solo-developer scope and the existing `SOT_Actor` struct, a full Entity-Component-System framework is not recommended. Instead, the existing struct shall be extended with:
- A property bag for flexible per-instance data.
- A template/archetype system for data-driven instantiation.
- Lua callbacks replacing hard-coded behavior.
- Optional sub-structs (e.g., `SOT_PlatformerController`, `SOT_BulletData`) attached via a tagged union or optional pointer, rather than a fully generic component registry.

This keeps the code straightforward while providing the flexibility needed for the three game types.

---

## 7. Animation System (`sot_animation` -- extend existing)

### 7.1 Purpose

Drives all visual animation in the engine: character movement cycles, environmental animation, cutscene sequences, and UI effects. The existing JSON-based frame system provides the foundation. This section defines enhancements that do not replace the existing format.

### 7.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| ANM-01 | Frame-based playback (existing) | T1 | Retain the existing system: animation sequences defined as arrays of atlas source rectangles, played at a configurable frame rate. |
| ANM-02 | Play modes | T1 | Support playback modes per-sequence: `loop`, `once` (play and stop on last frame), `ping-pong` (forward then reverse), `once-and-destroy` (play once then signal actor destruction). |
| ANM-03 | Frame events / tags | T1 | Attach named events to specific frames in an animation sequence. When playback reaches that frame, the event fires (dispatched to Lua and/or engine systems). Used for: sound triggers, projectile spawn, footstep dust, hit window start/end. |
| ANM-04 | Animation state machine | T2 | Define a state graph of animations with transitions. Each state references a sequence. Transitions define: trigger condition (action pressed, velocity threshold, Lua condition), blend/transition duration (instant or N frames), and whether the current animation must complete before transitioning. |
| ANM-05 | Named animation lookup | T1 | Query and set an actor's current animation by string name (e.g., `SOT_Animation_Play(actor, "idle_R")`), not by index. Names come from the JSON definition. |
| ANM-06 | Animation speed modifier | T2 | Per-instance playback speed multiplier (e.g., 0.5x for slow-motion, 2x for fast forward). Applied on top of the base `step_ms`. |

### 7.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| ANM-07 | Flexible sequence lengths and timing | T1 | PLAT | Support both short snappy animation cycles (4-8 frames for run/jump in action platformers) and long rotoscoped sequences (30+ frames for cinematic platformers). Variable per-frame durations enable both styles. |
| ANM-08 | Cutscene animation scripting | T2 | PLAT, ADV | Ability to drive animation state from Lua scripts: play a sequence, wait for it to finish, then play another. Coroutine-friendly: `play_animation("climb_up"); wait_animation(); play_animation("idle")`. |
| ANM-09 | Facial expressions / overlays | T3 | ADV | Support for composite sprites: a base body animation with an overlay sprite for expressions or held items, driven independently. |

### 7.4 Data Format

- **Animation definitions:** Extend the existing JSON format (`assets/animations/<name>.json`) with:
  - Per-sequence `play_mode` field (`"loop"`, `"once"`, `"ping_pong"`, `"once_destroy"`)
  - Per-frame optional `duration_ms` override (for variable-speed frames)
  - Per-frame optional `events` array: `[{"name": "footstep"}, {"name": "spawn_bullet", "data": {"offset_x": 8}}]`
- **Animation state machines:** Separate JSON file or embedded section in the animation definition. States reference sequence names, transitions reference actions or Lua conditions.

### 7.5 Integration Points

- **Rendering System:** Animation provides the current frame's atlas region to the sprite instance data each tick.
- **Audio System:** Frame events with audio names trigger sound effects.
- **Scripting (Lua):** Lua drives animation state (play/stop/set), receives frame event callbacks, and defines state machine transition conditions.
- **Entity/Actor System:** Each actor references one or more animation definitions and maintains playback state.

---

## 8. Physics and Collision (`sot_physics`, `sot_collider` -- extend existing)

### 8.1 Purpose

Provides spatial queries, collision detection, and physical simulation for all game objects. Box2D v3.1.1 (already fetched via CMake FetchContent) is the physics backend. Box2D 3.x is a pure-C rewrite with a clean API that natively handles collision filtering, sensors, continuous collision detection, and spatial queries — eliminating the need for custom implementations of these features.

### 8.2 Box2D Integration Strategy

**Backend:** Box2D v3.1.1 (pure C API, already linked)
**Role of cute_c2:** Retained for lightweight utility shape tests outside the physics world (e.g., UI hit-testing, editor gizmo intersection) but no longer the primary collision system.

**Key architectural decisions:**
- Each scene owns a `b2WorldId`. World is created on scene load, destroyed on scene unload.
- Fixed timestep for `b2World_Step()` (e.g., 1/60s) with accumulator pattern, decoupled from render frame rate (see TME-01).
- Pixel-to-meter scaling: define a constant (e.g., 16 pixels = 1 meter) to translate between pixel coordinates (rendering) and Box2D's meter-based units. All Box2D API calls use meters; conversion happens at the engine boundary.
- Gravity is a world property, configurable per-scene. Per-actor gravity scale via `b2Body_SetGravityScale()` for floating enemies, zero-G sections, etc.
- Actor-to-body mapping: each `SOT_Actor` with physics holds a `b2BodyId`. Body user data points back to the actor for callback resolution.

### 8.3 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| PHY-01 | Physics world management | T1 | Create/destroy a `b2WorldId` per scene. Step the world at fixed intervals. Sync actor transforms from Box2D body positions after each step. |
| PHY-02 | Body types | T1 | Support all three Box2D body types per actor: `b2_staticBody` (tilemap terrain, walls), `b2_kinematicBody` (moving platforms, scripted movers), `b2_dynamicBody` (player, enemies, projectiles). Configurable in actor templates. |
| PHY-03 | Collision filtering (layers/masks) | T1 | Use Box2D's built-in `b2Filter` (categoryBits, maskBits, groupIndex) for collision layers. Predefined categories: `TERRAIN`, `PLAYER`, `ENEMY`, `PLAYER_BULLET`, `ENEMY_BULLET`, `TRIGGER`, `COLLECTIBLE`, `NPC`. Configurable per-shape in actor templates. |
| PHY-04 | Sensors (trigger volumes) | T1 | Shapes marked as sensors (`b2ShapeDef.isSensor = true`) detect overlap without physical response. Box2D fires begin/end sensor events, dispatched as `on_trigger_enter` / `on_trigger_exit` to Lua. Used for: spawn triggers (SHMUP), checkpoints (PLAT), hotspots (ADV), room transitions. |
| PHY-05 | Collision events and callbacks | T1 | Use Box2D's contact events (begin/end contact, pre-solve for one-way platforms) to dispatch `on_collision(other, normal, impulse)` callbacks to Lua. Pre-solve callback enables per-contact filtering (e.g., one-way platform logic). |
| PHY-06 | Spatial query: point | T1 | `b2World_OverlapPoint()` — query which shapes contain a given point. Essential for mouse click detection in adventure games. |
| PHY-07 | Spatial query: raycast | T1 | `b2World_CastRay()` — cast a ray and return hits with normal and fraction. Used for line-of-sight, grounded detection, wall detection, laser weapons. Promoted to T1 since raycasts are the primary mechanism for grounded/wall detection in platformers. |
| PHY-08 | Spatial query: area (AABB/shape overlap) | T2 | `b2World_OverlapAABB()` / `b2World_OverlapShape()` — query all shapes overlapping a region. Used for area-of-effect attacks and proximity detection. |
| PHY-09 | Continuous collision detection | T1 | Enable Box2D's built-in CCD (`b2ShapeDef.enableContactEvents` + bullet body flag) for fast-moving objects. Prevents tunneling for bullets (SHMUP) and fast-dashing characters (PLAT). |
| PHY-10 | Shape types | T1 | Support Box2D shape types mapped from actor templates: `b2_circleShape`, `b2_capsuleShape`, `b2_polygonShape` (convex), `b2_segmentShape` (edges for terrain). |

### 8.4 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| PHY-11 | Gravity and grounded detection | T1 | PLAT | World gravity set per-scene. Grounded detection via downward raycast (PHY-07) or foot sensor. Per-actor gravity scale for variable jump: reduce gravity while jump held, restore on release. `b2Body_SetGravityScale()` per-frame in Lua controller. |
| PHY-11b | Wall detection and wall mechanics | T1 | PLAT | Side raycasts (PHY-07) detect wall contact. Wall-slide: override vertical velocity via `b2Body_SetLinearVelocity()` when wall-touching and falling. Wall-jump: apply impulse away from wall. Wall-cling: set velocity to zero while holding toward wall. All parameters (slide speed, jump force vector, cling duration) configurable in actor template. |
| PHY-11c | Dash mechanics | T2 | PLAT | Dash implemented by temporarily setting body to kinematic (or overriding velocity directly), moving at dash speed for a fixed duration, then restoring dynamic mode. Air dash count reset on grounding. Optional: disable collision response during dash for invincibility frames via collision filter group toggling. |
| PHY-12 | One-way platforms | T2 | PLAT | Implemented via Box2D pre-solve callback: disable contact when the actor is moving upward or is below the platform surface. Drop-through triggered by setting a per-actor flag that the pre-solve callback checks. |
| PHY-13 | Moving platforms | T2 | PLAT | Kinematic bodies moved via `b2Body_SetLinearVelocity()` along defined paths. Actors standing on them are carried automatically by Box2D's kinematic-dynamic interaction. |
| PHY-14 | Click/hover detection | T1 | ADV | Point query (PHY-06) with mouse position (converted to game/physics coordinates) to determine which interactive object the cursor is over. |
| PHY-15 | Pathfinding walkable area | T2 | ADV | Define a walkable polygon (or nav-mesh) per room. When the player clicks, compute a walk path along the walkable area. Simple polygon-based point-to-point pathfinding (not grid A*). This is separate from Box2D — uses geometric algorithms on Tiled-defined polygons. |

### 8.5 Data Format

- **Collider/shape definitions:** Part of actor templates (JSON). Shape type, dimensions, offset from actor origin, category/mask bits, sensor flag, body type, gravity scale, friction, restitution.
- **Tilemap colliders:** Extracted from Tiled object layers as static bodies with segment or polygon shapes. Layer/mask/sensor properties via Tiled custom properties.
- **Walkable areas (ADV):** Defined as polygons in Tiled object layers with a `"walkable"` type tag (not Box2D bodies — purely geometric).
- **Physics world config:** Per-scene in scene descriptor JSON: gravity vector, pixel-to-meter scale.

### 8.6 Integration Points

- **Entity/Actor System:** Each physics-enabled actor holds a `b2BodyId`. Actor creation/destruction creates/destroys the corresponding Box2D body. After `b2World_Step()`, actor positions are synced from body transforms.
- **Scripting (Lua):** Collision/sensor callbacks dispatched to Lua (`on_collision`, `on_trigger_enter`, `on_trigger_exit`). Lua can set velocities, apply impulses, toggle gravity scale, and perform spatial queries. Platformer controller logic (variable jump, wall-slide, dash) lives in Lua, calling physics API functions.
- **Scene System:** Scene creates/owns the `b2WorldId`. Tilemap colliders registered as static bodies. Scene bounds as static edge bodies.
- **Input System:** Coyote time (INP-09) reads the grounded state from the physics system's raycast results.
- **Debug Rendering:** Debug overlay draws Box2D shapes (bodies, sensors, contacts) using the existing debug line pipeline.

---

## 9. Scripting System -- Lua (`sot_lua` -- new)

### 9.1 Purpose

The gameplay logic layer. All game-type-specific behavior (enemy wave patterns, dialog trees, puzzle triggers, cutscene choreography) is authored in Lua. The C engine provides the runtime and APIs; Lua scripts define what happens. Hot-reload enables rapid iteration.

### 9.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| LUA-01 | Lua VM integration | T1 | Embed a Lua 5.4 VM (or LuaJIT). Initialize at engine startup. One VM instance for the entire engine. |
| LUA-02 | C-to-Lua API bindings | T1 | Expose engine subsystems to Lua as modules. Lua can call engine functions; the engine calls Lua callbacks. Bindings use Lua's C API directly (no binding generators for a C11 project). |
| LUA-03 | Actor script binding | T1 | Each actor template can reference a `.lua` script file. The script defines callback functions (`on_create`, `on_update`, `on_destroy`, `on_collision`, `on_trigger_enter`, `on_trigger_exit`). The engine invokes these at the appropriate lifecycle points. |
| LUA-04 | Hot-reload | T1 | When a `.lua` file is modified on disk, the engine detects the change and reloads the script without restarting. Actor state (properties bag) is preserved across reloads; only the function definitions are replaced. |
| LUA-05 | Error handling and reporting | T1 | Lua errors (syntax, runtime) are caught, logged to the console panel (in editor mode), and do not crash the engine. The offending actor's script is disabled until the next reload. |
| LUA-06 | Coroutine support | T2 | Support Lua coroutines for sequential scripting (cutscenes, dialog sequences). `wait(seconds)`, `wait_animation()`, `wait_dialog()` yield the coroutine and resume on the next frame when the condition is met. |
| LUA-07 | Global game state table | T1 | A persistent Lua table (`game_state` or similar) that survives scene transitions. Used for: inventory contents, quest flags, score, lives remaining. Serialized during save/load. |

### 9.3 Engine API Exposed to Lua

Organized by domain. Each becomes a Lua module (e.g., `sot.actor`, `sot.audio`, `sot.input`).

**Actor API:**
- `sot.actor.create(template_name, x, y, [properties])` -- Spawn actor from template
- `sot.actor.destroy(actor_id)` -- Remove actor
- `sot.actor.get_position(actor_id)` -- Returns x, y
- `sot.actor.set_position(actor_id, x, y)`
- `sot.actor.get_property(actor_id, key)`
- `sot.actor.set_property(actor_id, key, value)`
- `sot.actor.find_by_tag(tag)` -- Returns list of actor IDs
- `sot.actor.find_by_name(name)` -- Returns actor ID or nil

**Animation API:**
- `sot.anim.play(actor_id, animation_name)`
- `sot.anim.stop(actor_id)`
- `sot.anim.is_playing(actor_id)`
- `sot.anim.set_speed(actor_id, multiplier)`

**Audio API:**
- `sot.audio.play_music(filename, [fade_ms])`
- `sot.audio.stop_music([fade_ms])`
- `sot.audio.play_sfx(filename, [volume], [pitch])`
- `sot.audio.set_volume(channel, volume)` -- channel: "master", "music", "sfx"

**Input API:**
- `sot.input.is_pressed(action_name)`
- `sot.input.is_just_pressed(action_name)`
- `sot.input.get_mouse_position()` -- Returns x, y in game coordinates
- `sot.input.push_context(context_name)`
- `sot.input.pop_context()`

**Scene API:**
- `sot.scene.load(scene_name, [transition], [duration])`
- `sot.scene.get_current()`
- `sot.scene.set_camera_target(actor_id)`

**Physics API:**
- `sot.physics.set_velocity(actor_id, vx, vy)`
- `sot.physics.get_velocity(actor_id)`
- `sot.physics.apply_impulse(actor_id, ix, iy)`
- `sot.physics.point_query(x, y, layer_mask)` -- Returns list of actor IDs
- `sot.physics.raycast(x1, y1, x2, y2, layer_mask)` -- Returns first hit

**UI API:**
- `sot.ui.show_dialog(text, [portrait], [choices])`
- `sot.ui.hide_dialog()`
- `sot.ui.show_hud(element_name)`
- `sot.ui.hide_hud(element_name)`

### 9.4 Game-Type-Specific Lua APIs

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| LUA-08 | Shmup wave/pattern API | T2 | SHMUP | `sot.shmup.spawn_wave(wave_def)`, `sot.shmup.create_bullet_pattern(pattern_def)`, `sot.shmup.set_scroll_speed(speed)`. Wave definitions and bullet patterns are Lua tables describing timing, positions, and actor templates. |
| LUA-09 | Platformer control API | T2 | PLAT | `sot.plat.set_checkpoint(name)`, `sot.plat.respawn()`, `sot.plat.play_cutscene(script_name)`. Cutscenes use coroutines to sequence actor movements, camera pans, and dialog. |
| LUA-10 | Adventure game API | T2 | ADV | `sot.adv.add_to_inventory(item_id)`, `sot.adv.remove_from_inventory(item_id)`, `sot.adv.has_item(item_id)`, `sot.adv.use_item_on(item_id, target_actor_id)`, `sot.adv.walk_to(actor_id, x, y)`, `sot.adv.say(actor_id, text, [duration])`. |

### 9.5 Data Format

- **Lua scripts:** `.lua` files in `assets/scripts/`. One file per actor type or per system (e.g., `scripts/enemies/drone.lua`, `scripts/systems/wave_manager.lua`, `scripts/dialog/tavern.lua`).
- **Game state serialization:** The `game_state` Lua table is serialized to JSON for save files.

### 9.6 Integration Points

- **Every other subsystem:** Lua is the primary consumer of all engine APIs. It is the glue between subsystems.
- **Editor:** Console panel shows Lua output and errors. Script files can be opened in an external editor. Hot-reload triggered manually or on file change.

---

## 10. UI System -- In-Game (`sot_ui` -- new)

### 10.1 Purpose

Renders all in-game user interface elements: HUD, dialog boxes, menus, inventory screens, and interaction prompts. Distinct from the editor UI (ImGui); this system renders within the virtual framebuffer using the engine's pixel-art aesthetic.

### 10.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| UI-01 | UI layer rendering | T1 | UI elements render on a dedicated layer above all game content, using the UI camera (no parallax, no world transform). |
| UI-02 | Bitmap text rendering | T1 | Render text strings using bitmap fonts (see RND-06). Support for color, alignment (left, center, right), and line wrapping. |
| UI-03 | UI panel / box | T1 | Render 9-slice scalable panels (dialog boxes, inventory backgrounds, menu backgrounds) from a UI atlas. |
| UI-04 | Menu system | T1 | Vertical list menus with selectable items. Keyboard/gamepad navigation (up/down/confirm/cancel). Used for: title screen, pause menu, options, save/load. |
| UI-05 | Dialog box system | T1 | Display text in a dialog box with optional character portrait, character-by-character typewriter reveal, and support for player choices. Driven by Lua via `sot.ui.show_dialog()`. |
| UI-06 | Fade/transition overlay | T1 | Full-screen color overlay for fades. Shared with the screen transition system (RND-07). |

### 10.3 Game-Type-Specific Features

| ID | Feature | Tier | Game Type | Description |
|----|---------|------|-----------|-------------|
| UI-07 | HUD: score and lives | T1 | SHMUP | Persistent on-screen display of score, lives, and power-up level. Positioned at screen edges, not world-relative. |
| UI-08 | HUD: health/energy bar | T1 | PLAT | Horizontal bar showing health or energy, with configurable color gradient and flash-on-damage effect. |
| UI-09 | Inventory UI | T1 | ADV | Grid-based inventory panel showing collected items as icons. Click an item to select it; click a world object to use the selected item on it. |
| UI-10 | Verb bar / action bar | T1 | ADV | Row of action buttons: "Look at", "Pick up", "Use", "Talk to", "Open", "Close", "Push", "Pull" (configurable). Clicking a verb sets the current action mode; clicking a world object applies that verb. |
| UI-11 | Object name label | T2 | ADV | When hovering over an interactive object, display its name near the cursor or in a status bar. |

### 10.4 Data Format

- **UI layout definitions:** JSON files defining panel positions, sizes, font references, and 9-slice atlas regions. Path: `assets/ui/<layout>.json`.
- **Dialog content:** Authored in Lua scripts or JSON. Dialog trees with branches based on player choices or game state flags.
- **Bitmap font definitions:** JSON mapping characters to atlas regions (see RND-06 data format).

### 10.5 Integration Points

- **Rendering System:** UI system submits draw commands to the overlay render pipeline.
- **Input System:** UI elements consume input from the "menu" or "dialog" input context. When a dialog or menu is active, game input is blocked.
- **Scripting (Lua):** All UI content (dialog text, menu structure, HUD values) is driven from Lua. Lua calls show/hide, Lua receives choice callbacks.
- **Audio System:** UI interactions trigger sound effects (menu move, confirm, cancel, typewriter tick).

---

## 11. Editor -- Dear ImGui (`sot_editor` -- new)

### 11.1 Purpose

An embedded level/scene editor rendered inside the engine window using Dear ImGui. Provides a Unity-like workflow: edit scenes, inspect actors, preview animations, and toggle into play mode to test -- all without closing the application.

### 11.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| EDT-01 | ImGui integration | T1 | Integrate Dear ImGui (C bindings: cimgui) with the SDL3 GPU backend. ImGui renders at native window resolution, overlaying or beside the game's virtual framebuffer. |
| EDT-02 | Dockable panel system | T1 | ImGui docking branch. Panels can be docked, floated, tabbed, and resized. Layout persisted to an `imgui.ini` file. |
| EDT-03 | Play/stop toggle | T1 | A toolbar button (or hotkey) switches between editor mode and play mode. In editor mode, the game loop is paused; in play mode, the game runs normally. Entering play mode snapshots scene state; stopping restores the snapshot. |
| EDT-04 | Scene viewport | T1 | The virtual framebuffer is rendered into an ImGui image widget, acting as the scene viewport. In editor mode, the viewport supports panning and zooming the camera independently of game logic. |
| EDT-05 | Scene hierarchy panel | T1 | Tree view listing all actors in the current scene. Select an actor to inspect it. Supports reordering and parent-child relationships. |
| EDT-06 | Property inspector | T1 | When an actor is selected, display and edit its properties: transform (position, scale, rotation), physics (body type, velocity), animation (current sequence, frame), collider (shape, size, layer/mask), script reference, and custom properties. Changes are reflected immediately in the viewport. |
| EDT-07 | Asset browser | T2 | Panel listing available assets: textures, animations, sounds, scripts, scene files. Double-click to open or assign to the selected actor. |
| EDT-08 | Console / log panel | T1 | Scrolling log of engine messages, warnings, errors, and Lua output. Severity-colored. Filter by category. Optional Lua command input line for live evaluation. |
| EDT-09 | Animation preview panel | T2 | Select an animation definition and preview it: play, pause, scrub frame-by-frame, view frame events and per-frame duration. |
| EDT-10 | Gizmos | T2 | Visual handles in the scene viewport: position drag handle, collider bounds outline, trigger area shading, spawn point markers. |
| EDT-11 | Tilemap editor | T1 | Edit tilemaps directly in the engine. Features: tile palette panel showing available tiles from the tileset texture, paint/erase/fill tools, tile layer selection (background, midground, foreground), collision layer editing (mark tiles as solid, one-way, trigger). Reads and writes Tiled-compatible JSON format. Supports multiple tile layers per scene. Grid overlay with snap-to-tile placement. |
| EDT-12 | Spritesheet / atlas viewer | T1 | Panel that displays loaded spritesheet textures with a grid overlay matching the atlas frame definitions. Click a frame to see its ID, UV coordinates, and which animations reference it. Drag-and-drop frame assignment to animation sequences. Supports importing new spritesheets (PNG + JSON atlas descriptor) into the project. |
| EDT-13 | Spritesheet atlas editor | T2 | Define and edit atlas frame regions visually: draw rectangles over a spritesheet image to define frame boundaries, auto-detect frames by grid size or by color key, name frames, and export the atlas descriptor JSON. Useful when importing new art that doesn't come with a pre-made atlas. |
| EDT-14 | Tileset manager | T2 | Import and manage tilesets: load a tileset image, define tile size, configure per-tile properties (collision type, animation flag, custom properties). Tilesets are shared across scenes. |
| EDT-15 | Collision shape editor | T2 | Edit Box2D collision shapes visually in the viewport: draw/resize rectangles, circles, and polygons on actors or tilemap objects. Set shape properties (sensor, category/mask, friction, restitution) via the property inspector. Shapes update in the physics world immediately for live preview. |
| EDT-16 | Undo/redo | T3 | Command pattern undo/redo for property changes, tile painting, and actor placement in the editor. |

### 11.3 Data Format

- **Editor layout:** `imgui.ini` (Dear ImGui's built-in layout persistence).
- **Editor preferences:** JSON file for editor-specific settings (grid snap size, gizmo colors, default viewport zoom).
- **No separate editor file format:** The editor reads and writes the same scene descriptor JSON and actor template JSON files that the runtime uses.

### 11.4 Integration Points

- **Display System:** In editor mode, the virtual framebuffer renders into an ImGui texture rather than fullscreen. Editor UI surrounds it.
- **Scene System:** Editor loads, modifies, and saves scene descriptors. Tilemap editor modifies tilemap layers within the scene.
- **Entity/Actor System:** Editor creates, selects, moves, and configures actors. Inspector reads/writes actor properties.
- **Tilemap System:** Tilemap editor reads/writes Tiled-compatible JSON. Tile palette displays tileset textures. Collision layer edits create/modify static Box2D bodies.
- **Animation System:** Spritesheet viewer shows atlas frames and their animation references. Animation preview panel for playback testing.
- **Physics System (Box2D):** Collision shape editor creates/modifies Box2D shapes on actors and tilemap objects. Live preview of physics shapes in the viewport.
- **Scripting (Lua):** Console panel evaluates Lua expressions. Script errors displayed inline.
- **Input System:** Editor mode uses its own input context. Play mode switches to the game input context.
- **All subsystems:** Editor provides visibility into every subsystem's state for debugging.

---

## 12. Asset Pipeline

### 12.1 Purpose

Defines the conventions and tooling around how game assets are created, stored, processed, and loaded at runtime. The existing codebase uses a straightforward "files in directories" approach with path resolution via `SOT_Paths`. This section formalizes that and adds hot-reload support.

### 12.2 Core Features

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| AST-01 | Directory conventions | T1 | Standardized asset directory structure under `assets/`: `textures/`, `animations/`, `maps/`, `scenes/`, `scripts/`, `shaders/source/`, `shaders/compiled/SPIRV/`, `audio/music/`, `audio/sfx/`, `fonts/`, `actors/`, `ui/`. |
| AST-02 | Supported formats | T1 | **Images:** PNG (RGBA). **Maps:** JSON (Tiled export). **Animations:** JSON (engine format). **Actors:** JSON (engine format). **Scripts:** Lua (.lua). **Audio:** OGG Vorbis, WAV. Tracker: MOD, XM, IT. **Fonts:** PNG atlas + JSON descriptor. **Scenes:** JSON (engine format). **Shaders:** GLSL source + compiled SPIR-V. |
| AST-03 | Asset hot-reload (editor mode) | T1 | When in editor mode, monitor asset files for changes. On modification: reload textures (re-upload to GPU), reload Lua scripts (re-execute), reload JSON data (re-parse). Game does not need to restart. |
| AST-04 | Spritesheet packing | T3 | Integration with external tools (TexturePacker, Aseprite export) or a built-in offline packer that combines individual sprite images into atlas PNGs with corresponding JSON frame data. |
| AST-05 | Shader compilation workflow | T1 | Document and optionally automate the GLSL-to-SPIR-V compilation step (currently manual `glslc` invocation). A CMake custom command or build script that re-compiles modified shaders. |
| AST-06 | Build-time asset sync (existing) | T1 | Retain the existing CMake `CopyAssets` target that syncs `assets/` to the build directory on every build. |

### 12.3 Data Format

All data formats are specified in their respective subsystem sections. This section establishes the overarching convention: **JSON for all structured data, PNG for all images, Lua for all scripts, OGG/WAV for all audio.** No custom binary formats are required for the initial engine. Binary packing of assets for distribution is a T3 concern.

### 12.4 Integration Points

- **Every subsystem:** Each subsystem loads assets from its designated directory using `SOT_Paths` for resolution.
- **Editor:** Asset browser enumerates files from these directories. Hot-reload watches these directories.

---

## 13. Cross-Cutting Concerns

### 13.1 Error Handling

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| ERR-01 | Consistent error reporting | T1 | All subsystems report errors via `SDL_Log` with severity levels. No silent failures. JSON parse errors, file-not-found, GPU failures, and Lua errors all produce logged messages. |
| ERR-02 | Graceful degradation | T2 | Missing assets use fallback placeholders (magenta checkerboard for missing textures, silent for missing audio, default collider for missing shapes) rather than crashing. |

### 13.2 Memory Management

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| MEM-01 | SDL allocator consistency | T1 | Continue using `SDL_malloc`/`SDL_calloc`/`SDL_free` for all engine allocations. |
| MEM-02 | Complete cleanup on shutdown | T1 | `SDL_AppQuit` (recently fixed) shall free all GPU resources, textures, buffers, scenes, actors, audio handles, and Lua VM. No leaks on clean exit. |

### 13.3 Timing

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| TME-01 | Fixed timestep update | T1 | Game logic and Box2D `b2World_Step()` run at a fixed timestep (e.g., 1/60s) with accumulator pattern. Rendering interpolates between states. Required for deterministic physics behavior. |
| TME-02 | Delta time (existing) | T1 | Retain the existing `deltaTime` calculation in `SDL_AppIterate`. Subsystems that do not use fixed timestep use this directly. |

### 13.4 Debug Tooling

| ID | Feature | Tier | Description |
|----|---------|------|-------------|
| DBG-01 | Collider visualization (existing) | T1 | Retain and enhance the debug overlay for collider shapes. Add collision layer coloring. |
| DBG-02 | Trigger volume visualization | T2 | Render trigger volumes with semi-transparent fill in debug mode. |
| DBG-03 | Frame rate counter | T1 | Display current FPS in the editor console or as an optional in-game overlay. |
| DBG-04 | Lua debug print | T1 | `print()` in Lua outputs to the editor console panel. |

---

## 14. Summary: Feature Count by Tier

| Tier | Count | Description |
|------|-------|-------------|
| T1 | ~70 | Foundation: every game type needs these |
| T2 | ~45 | Game-type-specific or important polish |
| T3 | ~6 | Nice-to-have, deferrable |

### Recommended Implementation Sequence

1. **Display System** (DSP-01 through DSP-05) -- virtual framebuffer is the foundation everything renders to
2. **Input System** (INP-01 through INP-05) -- abstract input before building gameplay
3. **Physics System / Box2D** (PHY-01 through PHY-10) -- integrate Box2D world, body types, collision filtering, sensors, raycasts. Replace cute_c2 as primary collision backend.
4. **Entity/Actor refactoring** (ENT-01 through ENT-04) -- templates, lifecycle callbacks, Box2D body ownership
5. **Scripting System / Lua** (LUA-01 through LUA-07) -- gameplay logic layer
6. **Animation enhancements** (ANM-02, ANM-03, ANM-05) -- play modes, frame events, named lookup
7. **Audio System** (AUD-01, AUD-03, AUD-04, AUD-07) -- basic music and SFX
8. **Scene System enhancements** (SCN-01 through SCN-04) -- scene manager, transitions, Box2D world per scene
9. **Rendering enhancements** (RND-01, RND-02, RND-06, RND-10) -- layers, parallax, fonts, flip
10. **UI System** (UI-01 through UI-06) -- in-game UI
11. **Editor** (EDT-01 through EDT-16) -- ImGui integration, tilemap editor, spritesheet viewer, collision shape editor
12. **Game-type-specific features** (T2 items per genre: platformer mechanics, shmup patterns, adventure UI)

---

### Critical Files for Implementation

- `E:/Coding/sdl-prototype/src/includes/sot_scene.h` -- Scene struct and descriptor; the central data structure that must be extended for layers, camera modes, and multi-scene management
- `E:/Coding/sdl-prototype/src/includes/sot_actor.h` -- Actor struct; must be refactored for templates, property bags, lifecycle callbacks, and Lua binding
- `E:/Coding/sdl-prototype/src/includes/sot_gpu_pipeline.h` -- GPU state and pipeline definitions; must be extended for the virtual framebuffer, layer compositing, and new render passes (particles, text, transitions)
- `E:/Coding/sdl-prototype/src/main.c` -- Application entry point and main loop; must accommodate editor toggle, fixed timestep, input system initialization, and Lua VM startup
- `E:/Coding/sdl-prototype/src/includes/sot_animation.h` -- Animation structures; must be extended for play modes, frame events, and state machines