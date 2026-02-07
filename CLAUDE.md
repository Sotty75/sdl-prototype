# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C11 2D game engine prototype using SDL3's GPU backend (Vulkan). Implements a platformer with tilemap levels, sprite animation, and collision detection.

## Build Commands

**Toolchain:** MinGW-w64 via MSYS2 (UCRT64), CMake 3.21+, Ninja generator.

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -B build -G Ninja

# Build (incremental)
cmake --build build

# Clean rebuild
cmake --build build --clean-first
```

The build produces `build/sdl-prototype.exe`. Assets are auto-synced to `build/assets/` and DLLs (SDL3, SDL3_image) are auto-copied on each build. Dependencies (SDL3, SDL3_image, Box2D) are fetched from source via CMake FetchContent.

**Shader compilation** (GLSL to SPIR-V):
```bash
glslc assets/shaders/source/<shader>.vert -o assets/shaders/compiled/SPIRV/<shader>.vert.spv
glslc assets/shaders/source/<shader>.frag -o assets/shaders/compiled/SPIRV/<shader>.frag.spv
```

**Run:** `build/sdl-prototype.exe` (working directory should be `build/` or assets won't be found).

**Debug:** VSCode launch config uses GDB at `C:\msys64\ucrt64\bin\gdb.exe`.

## Architecture

### Application Loop

Uses SDL3's callback model (no traditional main loop):
- `SDL_AppInit()` — initialization, scene loading, GPU setup
- `SDL_AppEvent()` — input handling (keyboard, gamepad)
- `SDL_AppIterate()` — per-frame update + render
- `SDL_AppQuit()` — cleanup (currently incomplete)

Entry point is [main.c](src/main.c).

### Global State

`AppState` (defined in [appstate.h](src/includes/appstate.h)) holds everything: GPU state, texture pool (linked list), collider lists (static for tilemap, dynamic for actors), debug toggles, and timing.

### Module Naming

All modules use `sot_` prefix. Headers in `src/includes/`, implementations in `src/`. The umbrella header [sot_engine.h](src/includes/sot_engine.h) includes all subsystems.

### Rendering (sot_gpu_pipeline)

Multi-pipeline GPU rendering via SDL3's GPU API (Vulkan backend):
- **SOT_RP_TILEMAP** — single draw call for entire tilemap, vertex shader computes tile positions/UVs
- **SOT_RP_SPRITE** — instanced sprite rendering via storage buffers (SSBO)
- **SOT_RP_OVERLAY** — UI layer
- **SOT_RP_DEBUG** — collision shape wireframes (toggle with `1` key)

CGLM configured with `CGLM_FORCE_LEFT_HANDED` and `CGLM_FORCE_DEPTH_ZERO_TO_ONE`.

### Scene / Actor / Tilemap

- **SOT_Scene** — contains a fixed-size actor array (2000 max), a tilemap, and world/UI cameras
- **SOT_Actor** — entity with transform, physics, animations (up to 256), collider, and GPU sprite instance
- **sot_tilemap** — loaded from Tiled editor JSON files via `cute_tiled.h`; includes collision shapes extracted from map data

### Collision

Uses `cute_c2.h` (not Box2D, which is included but not integrated). Supports circle, capsule, AABB, and polygon shapes. Colliders are stored in linked lists: static (tilemap) and dynamic (actors).

### Asset Paths

`SOT_Paths` global struct (in [sot_common.h](src/includes/sot_common.h)) resolves paths relative to the executable: `Textures`, `Shaders`, `TiledMaps`, `Animations`.

### Texture Management

Texture pool is a linked list. `GetTexture()` creates on first request and caches for reuse.

## Third-Party Libraries

| Library | Location | Purpose |
|---------|----------|---------|
| SDL3 / SDL3_image | FetchContent | Windowing, GPU, image loading |
| Box2D 3.1.1 | FetchContent | Physics (included, not yet integrated) |
| cglm | `include/cglm/` | Vector/matrix math (header-only) |
| cJSON | `include/cJSON/` | JSON parsing |
| cute_tiled | `include/cute_headers/` | Tiled map loading (header-only) |
| cute_c2 | `include/cute_headers/` | 2D collision detection (header-only) |

## Key Conventions

- New `.c` source files are auto-discovered by CMake via `GLOB_RECURSE` on `src/*.c`
- Structs use PascalCase (`SOT_Actor`) or snake_case with prefix (`sot_tilemap`)
- Functions use snake_case with module prefix
- Animation data and tilemaps are defined in JSON files under `assets/`
