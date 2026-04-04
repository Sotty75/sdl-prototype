# Repository Guidelines

## Project Structure & Module Organization
`src/` contains the engine and gameplay implementation in C. Core modules follow the `sot_*.c` pattern, and public headers live in `src/includes/`. `include/` stores bundled third-party headers and sources such as `cglm`, `cJSON`, and `cute_headers`; treat these as vendored dependencies unless you are intentionally updating them. `assets/` contains runtime data: maps, scenes, Lua scripts, textures, fonts, and shader source/compiled binaries. `build/` is generated output from CMake/Ninja and should not be edited by hand.

## Build, Test, and Development Commands
Configure once:
```powershell
cmake -S . -B build -G Ninja
```
Build the executable and sync assets into `build/assets`:
```powershell
cmake --build build
```
Run the current build:
```powershell
.\build\sdl-prototype.exe
```
Recompile a shader after editing files in `assets/shaders/source/`:
```powershell
glslc assets\shaders\source\shader.vert -o assets\shaders\compiled\SPIRV\shader.vert.spv
```
The project fetches SDL3, SDL3_image, Box2D, Lua, and cimgui through CMake `FetchContent`, so the first configure may take longer.

## Coding Style & Naming Conventions
Target C11 as defined in `CMakeLists.txt`. Follow the existing style in `src/`: 4-space indentation, braces on the same line for functions and control blocks, and `sot_` prefixes for module files and headers. Use `SOT_` for exported types, macros, and API-style function groups. Prefer descriptive snake_case for local variables and file names. Keep comments brief and only where the control flow or rendering setup is not obvious.

## Testing Guidelines
There is no dedicated `tests/` directory or `ctest` target yet. Validate changes by rebuilding with `cmake --build build` and running `.\build\sdl-prototype.exe`. For rendering, input, scene-loading, or shader work, include a short manual test note in the PR describing what you exercised.

## Commit & Pull Request Guidelines
Recent history uses short, imperative commit subjects such as `Fixed major coding issues` and `Added helper to build assets paths as global variables.` Keep commits focused and summarize the behavior change, not the implementation detail. Pull requests should include: a concise description, any linked issue or task, manual test coverage, and screenshots or clips for editor/rendering changes.

## Assets & Configuration
Do not hardcode machine-specific paths. If you add assets, keep directory names and references consistent with the existing `assets/` layout so CMake's asset copy step picks them up automatically.
