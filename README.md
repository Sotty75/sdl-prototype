# sdl-prototype

A C prototype project built using **SDL3** and **SDL3_image**.
The build system relies on **CMake** with the **Ninja** generator, compiled via the **MinGW-w64 (MSYS2)** toolchain.

## Prerequisites

Before building, ensure you have the following environment set up on Windows:

1.  **MSYS2** installed (UCRT64 environment recommended).
2.  The following packages installed via `pacman` in MSYS2:
    * `mingw-w64-ucrt-x86_64-gcc` (Compiler)
    * `mingw-w64-ucrt-x86_64-cmake` (Build System)
    * `mingw-w64-ucrt-x86_64-ninja` (Fast Generator)
    * `mingw-w64-ucrt-x86_64-gdb` (Debugger)

## Library Configuration

This project requires **SDL3** and **SDL3_image**. Since these libraries are referenced via custom absolute paths, you **must update** `CMakeLists.txt` to point to the correct locations on your machine.

Open `CMakeLists.txt` and modify the following variables:

## Current Dependencies

The DLL of the following libraries should be copied in the build folder once your solution is setup.
You need also the libraries themselves (header files) available in your include paths.

- SDL3
- SDL_Image

## Shaders compilation

```
PS C:\Coding\sdl-prototype\assets\shaders\source> glslc C:\coding\sdl-prototype\assets\shaders\source\shader.vert -o shader.vert.spv
```

```
PS C:\Coding\sdl-prototype\assets\shaders\source> glslc C:\coding\sdl-prototype\assets\shaders\source\shader.frag -o shader.frag.spv
```

## SDL Reference Links

[SDL3](https://wiki.libsdl.org/SDL3/FrontPage)
[SDL_Image Quick Reference](https://wiki.libsdl.org/SDL3_image/QuickReference)
