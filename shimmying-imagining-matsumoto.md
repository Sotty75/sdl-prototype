# Plan: In-Engine Unity-like Editor

## Context

The SDL3/Vulkan game engine prototype has reached a state where all core subsystems exist
(rendering, tilemaps, actors, animations, collisions) but authoring everything requires
manually editing JSON files and restarting the game. The goal is to build an embedded
editor — similar to Unity Editor — that lets you author scenes, actors, tilemaps,
animations, and shaders interactively, with all changes written back to the existing JSON
asset files. The editor is a separate CMake build target (`sdl-prototype-editor`) compiled
with `-DSOT_EDITOR`; the shipping game target (`sdl-prototype`) is completely unaffected.

---

## Key Technology Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| UI framework | **cimgui** (C bindings for Dear ImGui) | Pure C API, rich widgets (docking, tables, image display, drag-drop), industry standard for game editors. Nuklear lacks the required widget depth. |
| Editor integration | **Embedded mode** (`#ifdef SOT_EDITOR`) | Direct access to all live engine structs (actor physics state, current animation frame, colliders). No IPC overhead. Toggle with F1. |
| Viewport rendering | **Offscreen texture** → `igImage` | Game renders to `SDL_GPUTexture` (COLOR_TARGET + SAMPLER), ImGui displays it in a docked window. |
| Tile authoring | **Custom painter writing Tiled JSON** | Keeps the `cute_tiled` load pipeline intact. The editor writes the same `level_00.json` format the engine already reads. |
| Shader live reload | **glslc via `SDL_CreateProcess`** + pipeline hot-swap | Already on PATH in MSYS2. Safe: new pipeline fully constructed before old one is released. |
| JSON serialization | **cJSON** (already in project) | `include/cJSON/cJSON.c` is already compiled. Use `cJSON_CreateObject` / `cJSON_Print` for all writes. |

---

## Architecture Overview

```
src/
├── main.c                    ← add #ifdef SOT_EDITOR lifecycle hooks
├── editor/                   ← new directory, all compiled only into editor target
│   ├── sot_editor.h/.c           Core editor state + init/update/shutdown
│   ├── sot_editor_viewport.h/.c  Offscreen game view, camera pan, selection
│   ├── sot_editor_hierarchy.h/.c Scene tree panel
│   ├── sot_editor_inspector.h/.c Property editor for selected object
│   ├── sot_editor_tilemap.h/.c   Tile painter + collision shape editor
│   ├── sot_editor_animation.h/.c Animation preview + frame editor
│   ├── sot_editor_shader.h/.c    GLSL text editor + live compile
│   ├── sot_editor_assets.h/.c    Asset browser with drag-drop
│   ├── sot_editor_serializer.h/.c JSON save (scene, tilemap, animation)
│   └── sot_imgui_backend.cpp     Thin C++ file compiling imgui_impl_sdl3 + imgui_impl_sdlgpu3
```

**`AppState`** gets `SOT_Editor *editor` field under `#ifdef SOT_EDITOR` — gives every
module editor selection state without circular includes.

**`SOT_GPU_State`** gets `SDL_GPUTexture *editorViewportTexture` under `#ifdef SOT_EDITOR`
for the offscreen render target.

---

## Milestone Roadmap

### M0 — Fix Sprite Rendering *(blocker for M2+)*
**Complexity: S | Engine changes only, no editor code**

Fix the two known bugs before any editor work:
1. `assets/shaders/source/shaderSprite.vert` — `model[3][0]` written twice (Y never set); `texCoord` undeclared (must be `outTexCoord`)
2. `src/sot_scene.c` — implement `SOT_GPU_RenderActors()` draw call body; fill `gpuSpritesInfo[]` in `UpdateScene`; move SSBO upload out of render

Critical files: `shaderSprite.vert`, `shaderSprite.frag`, `src/sot_scene.c`

---

### M1 — cimgui Bootstrap
**Complexity: M | Purely additive — zero changes to existing engine files except `main.c` guards**

1. **CMakeLists.txt**: add `LANGUAGES C CXX`; FetchContent cimgui; add `sdl-prototype-editor` target (same `SOURCE_FILES` glob + `src/editor/*.c` + `src/editor/*.cpp`); link cimgui; add `-DSOT_EDITOR`
2. **`src/editor/sot_imgui_backend.cpp`**: compile `imgui_impl_sdl3.cpp` + `imgui_impl_sdlgpu3.cpp` (Dear ImGui ships both SDL3 + SDL_GPU backends)
3. **`src/editor/sot_editor.h/.c`**: `SOT_EditorInit()`, `SOT_EditorUpdate()` (calls `igNewFrame`, `igShowDemoWindow`, `igRender`, submits draw data), `SOT_EditorShutdown()`
4. **`src/main.c`** under `#ifdef SOT_EDITOR`:
   - `SDL_AppInit`: call `SOT_EditorInit(as)` after GPU ready
   - `SDL_AppEvent`: forward to `igImplSDL3ProcessEvent(event)`
   - `SDL_AppIterate`: call `SOT_EditorUpdate(as, currentScene)` after `SOT_GPU_Render`
   - `SDL_AppQuit`: call `SOT_EditorShutdown()`

*Deliverable: game runs normally; editor target shows ImGui demo window on top.*

---

### M2 — Offscreen Viewport
**Complexity: M | One refactor in `sot_gpu_pipeline.c`, guarded by `#ifdef SOT_EDITOR`**

1. **`src/includes/sot_gpu_pipeline.h`**: add `SDL_GPUTexture *editorViewportTexture` + `SDL_GPUSampler *editorViewportSampler` to `SOT_GPU_State` under `#ifdef SOT_EDITOR`
2. **`src/sot_gpu_pipeline.c`** — `SOT_GPU_Render`:
   - In editor mode: create offscreen texture at init (`SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SAMPLER`); redirect game render pass to it; second render pass targets swapchain for ImGui
   - Non-editor path unchanged
3. **`src/editor/sot_editor_viewport.h/.c`**: `SOT_EditorViewport_Draw()` — `igImage` of viewport texture (registered via `ImGui_ImplSDLGPU3_AddTexture`); mouse drag → `UpdateCamera`; store viewport screen rect for world↔screen coord transforms

*Deliverable: game world visible inside an ImGui docked window; camera pannable with mouse drag.*

---

### M3 — Hierarchy + Inspector + Save
**Complexity: L | All new files in `src/editor/`, no engine changes**

1. **`src/editor/sot_editor_hierarchy.h/.c`**: `SOT_EditorHierarchy_Draw()` — `igSelectable` per actor + tilemap entry; sets `editor->selectedActorIndex`
2. **`src/editor/sot_editor_inspector.h/.c`**: `SOT_EditorInspector_Draw()`:
   - `igDragFloat2("Position")` / Scale / Rotation — write directly into `actor->transform`
   - `igCombo("Body Type")` → `SOT_PHYSICS_BODY` enum
   - `igDragFloat2("Velocity")`
   - `igCombo("Collider Type")` → `C2_TYPE_*`; conditional shape fields (radius, min/max, etc.)
   - Animations list with `igCollapsingHeader` per sequence
3. **`src/editor/sot_editor_serializer.h/.c`**: `SOT_EditorSaveScene()` — extend scene descriptor JSON to include `actors[]` array (name, animationFile, startPosition, physics, collider type). This makes actor loading data-driven — matching the existing `sot_scene.c` TODO.
4. **Viewport gizmo**: `igDrawList` arrows over selected actor, draggable → modifies `transform.pos`

*Extend `assets/scenes/scene_00_.json` schema: add `actors[]`. `SOT_LoadScene` + `SOT_InitializeScene` updated to load actors from descriptor (existing hardcoded actor[0] logic removed — this is the TODO already in the code).*

---

### M4 — Tilemap Painter
**Complexity: L | One new function in `sot_tilemap.c`, rest additive**

1. **`src/sot_tilemap.c`**: add `SOT_GPU_InvalidateTilemap(sot_tilemap *tm, SOT_GPU_State *gpu)` — re-uploads only `SOT_TILEMAP_SSB` without reallocating textures/vertex buffers. Declare in `sot_tilemap.h`.
2. **`src/editor/sot_editor_tilemap.h/.c`**: `SOT_EditorTilemapPainter_Draw()`:
   - Tileset atlas `igImage` with selection grid overlay via `igGetWindowDrawList`
   - Paint/Erase/Fill toolbar
   - On viewport click (while tool active): unproject screen→world via `glm_mat4_inv` + cglm unproject; compute tile index; write `scene->tilemap->tiles[idx]`; call `SOT_GPU_InvalidateTilemap`
   - Collision layer sub-panel: add/remove/resize AABB/poly shapes; visualized via existing debug draw infrastructure (`SOT_GPU_AddLine`)
3. **`src/editor/sot_editor_serializer.c`**: add `SOT_EditorSaveTiledMap()` — writes `layers[0].data[]` (tile array) and `layers[1].objects[]` (collision shapes) in Tiled JSON format

---

### M5 — Animation Editor
**Complexity: M | Purely additive**

**`src/editor/sot_editor_animation.h/.c`**: `SOT_EditorAnimation_Draw()`:
- Left: atlas `igImage` with frame grid overlay; click to toggle frames into/out of the current sequence
- Right: sequence list (`igListBox`), frame order list, `igDragInt("Frame ms")`
- Preview: small `igImage` cycling frames using `SDL_GetTicks()` % `(count * step_ms)` → correct UV from `uv0`/`uv1` params (no GPU pass needed — ImGui crops the already-uploaded atlas texture)
- Save: `SOT_EditorSaveAnimation()` writes back `monkey.json` format via cJSON
- After save: call `SOT_ActorBindAnimations(actor, editedInfo)` to propagate to runtime actor

---

### M6 — Shader Editor + Live Reload
**Complexity: L | One new function in `sot_gpu_pipeline.c` (editor-only), rest additive**

1. **`src/sot_gpu_pipeline.c`**: add `SOT_GPU_HotReloadPipeline(SOT_GPU_State *gpu, SOT_PipelineID id, SOT_GPU_PipelineInfo *info)` under `#ifdef SOT_EDITOR`:
   - Save old: `SDL_GPUGraphicsPipeline *old = gpu->pipeline[id]`
   - Call `SOT_GPU_InitPipelineWithInfo(gpu, info)` → writes new pipeline into slot
   - Release old: `SDL_ReleaseGPUGraphicsPipeline(gpu->device, old)` (safe — called after frame submit)
2. **`src/editor/sot_editor_shader.h/.c`**: `SOT_EditorShader_Draw()`:
   - Dropdown mapping pipeline IDs to shader filenames
   - Reads source from `assets/shaders/source/` into `char buf[64*1024]`
   - `igInputTextMultiline` for editing
   - "Save" → write buffer to source file
   - "Compile & Reload" → `SDL_CreateProcess` running `glslc <source> -o <spv>`; capture stderr; on success call `SOT_GPU_HotReloadPipeline`; on failure show stderr in red text panel

---

### M7 — Asset Browser + Full Round-Trip
**Complexity: M | Purely additive**

1. **`src/editor/sot_editor_assets.h/.c`**: bottom panel with tabs (Textures, Animations, Maps, Scenes); `SDL_EnumerateDirectory` per folder; texture tab shows `igImage` thumbnails; drag-drop sources (`igBeginDragDropSource` with asset path payload)
2. Hierarchy actors + viewport register as drop targets: dropping an animation `.json` calls `SOT_LoadAnimations` + `SOT_ActorBindAnimations`
3. **Serializer completion**: `SOT_EditorSaveScene` writes the full extended descriptor; `SOT_EditorLoadScene` calls `SOT_InitializeScene` and resets editor state — full round-trip verified: edit → save → restart → load reproduces state

---

## Milestone Summary

| # | Goal | Complexity | Engine files modified | Breaking |
|---|------|------------|----------------------|---------|
| M0 | Fix sprite shader + render | S | `shaderSprite.vert`, `sot_scene.c` | No |
| M1 | cimgui bootstrap | M | `main.c` (#ifdef only), `CMakeLists.txt` | No |
| M2 | Offscreen viewport | M | `sot_gpu_pipeline.c` (#ifdef only) | No |
| M3 | Hierarchy + Inspector + Save | L | `sot_scene.c` (actor loading TODO) | No |
| M4 | Tilemap painter | L | `sot_tilemap.c` (add 1 fn) | No |
| M5 | Animation editor | M | None | No |
| M6 | Shader editor + hot-reload | L | `sot_gpu_pipeline.c` (add 1 fn, #ifdef) | No |
| M7 | Asset browser + round-trip | M | None | No |

Every milestone is independently deployable. No milestone breaks the shipping game target.
The first line of code to write: `shaderSprite.vert` line 44 (`model[3][1]` fix).

---

## Verification Per Milestone

- **M0**: Build + run `sdl-prototype`; player actor renders on screen with correct position and animation
- **M1**: Build `sdl-prototype-editor`; ImGui demo window appears over running game with no frame rate impact on `sdl-prototype`
- **M2**: Game world visible inside docked ImGui window; mouse drag pans camera; tilemap and actors render correctly inside the viewport texture
- **M3**: Select actor in Hierarchy → Inspector shows its transform; drag Position → actor moves in viewport; Save → JSON file updated; restart loads the saved position
- **M4**: Select a tile in the painter → click in viewport → tile changes visually; save → Tiled JSON updated; restart shows painted tiles
- **M5**: Play button cycles animation frames in preview panel; edit frame list → actor in viewport updates; save → `monkey.json` updated
- **M6**: Edit a shader line → Compile & Reload → visual change appears in viewport within the same session; error shows on bad GLSL
- **M7**: Drag animation file onto actor in Hierarchy → actor changes spritesheet; full save + restart reproduces the scene exactly
