# SDL Prototype Architecture

This document outlines the architecture of the SDL-based 2D game prototype.

## Core Concepts

The application is built in C and uses the **SDL3** library for windowing, input, and the main application loop. It follows a modern SDL3 approach, using callbacks (`SDL_AppInit`, `SDL_AppIterate`, etc.) instead of a traditional `main` function with a while loop.

A central `AppState` struct holds the global state of the application, including the GPU state, timing information, and pointers to global systems like the texture pool and collider lists.

### Rendering

The rendering is handled by a custom GPU abstraction layer, prefixed with `sot_gpu_`. This layer is responsible for all communication with the graphics API. The CGLM library is configured for a left-handed coordinate system with a depth range of 0 to 1, which strongly suggests the use of a modern graphics API like **Vulkan**.

The renderer has dedicated paths for different types of objects:
- **Tilemaps:** Tilemaps are rendered efficiently on the GPU using a vertex shader that calculates tile positions and texture coordinates on the fly. This allows for large maps to be drawn with a single draw call.
- **Sprites:** Sprites are used for actors and other game objects.
- **Debug Geometry:** The renderer can draw debug information, such as collider shapes.

### Game Objects

- **`SOT_Scene`**: Represents a single screen or level in the game. It holds the player, the tilemap, and cameras.
- **`SOT_Actor`**: Represents a dynamic game object. Actors have properties like position, velocity, and animation. They also have a collider for physics.
- **`sot_tilemap`**: Represents a tilemap loaded from a Tiled editor `.json` file. It contains the tile data as well as collision information extracted from the map.

### Physics and Collision

The project uses `cute_c2.h` for 2D collision detection between actors and the tilemap. The `sot_collider` module manages the collision shapes.

While the project includes `box2d` as a dependency, its usage is not apparent from the main application logic. The primary collision system for actors and tilemaps seems to be the custom implementation using `cute_c2`.

## Modules

The codebase is organized into several modules with the `sot_` prefix:

- **`sot_actor`**: Manages game actors.
- **`sot_camera`**: Manages the world and UI cameras.
- **`sot_collider`**: Manages collision shapes and detection.
- **`sot_gpu_pipeline`**: The core of the rendering abstraction layer.
- **`sot_quad`**: A basic renderable primitive (likely used by sprites and other objects).
- **`sot_scene`**: Manages the game scene.
- **`sot_animation`**: Manages animated sprites.
- **`sot_texture`**: Manages textures.
- **`sot_tilemap`**: Manages tilemaps.

## External Libraries

- **SDL3**: Windowing, input, application loop.
- **SDL3_image**: Loading image files.
- **cglm**: A math library for C, used for vector and matrix operations.
- **cJSON**: For parsing JSON files (used for maps and animations).
- **cute_tiled**: For loading `.json` maps from the Tiled editor.
- **cute_c2**: For 2D collision detection.
- **box2d**: A 2D physics engine (included but not fully integrated into the main game logic).

# TODO List

- **Integrate Box2D:** The `box2d` library is included but not used for the main actor physics. The current custom collision system could be replaced or augmented with `box2d` for more advanced physics simulations (e.g., rigid body dynamics, joints).
- **Scene Management:** The current implementation only has a single, hardcoded scene. A more robust scene manager could be implemented to allow for multiple levels and transitions between them.
- **Entity Management:** The current scene has a single `player` actor. A more general entity management system (like an Entity-Component-System, or ECS) could be used to allow for more flexible and data-driven creation of game objects.
- **UI System:** The UI is currently minimal. A dedicated UI system could be added for menus, HUD elements, etc.
- **Audio:** There is no audio system. SDL3's audio capabilities could be used to add sound effects and music.
- **Cleanup:** The `SDL_AppQuit` function has most of its cleanup code commented out. This should be fixed to prevent memory leaks and ensure a clean shutdown.
- **Error Handling:** Add more robust error handling throughout the application.
