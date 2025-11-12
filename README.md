# sdl-prototype
Test Project with some experiments using the SDL library.


The idea of this project is to use it to experiment with the possibilities of the SDL library.
And take advantage of it to practice with C language.
Also this serves as an example of ho to configure a C project using the CMAKE tool and build and debug it in VSCode.

## Build

```
mdir build
cd build
cmake -G "MinGW Makefiles" ..
```

## Current Dependencies

The DLL of the following libraries should be copied in the build folder once your solution is setup.
You need also the libraries themselves (header files) available in your include paths.

- SDL3
- SDL_Image

## Shaders compilation

```
PS F:\Coding\sdl-prototype\assets\shaders\source> glslc f:\coding\sdl-prototype\assets\shaders\source\shader.vert -o shader.vert.spv
```

```
PS F:\Coding\sdl-prototype\assets\shaders\source> glslc f:\coding\sdl-prototype\assets\shaders\source\shader.frag -o shader.frag.spv
```

## SDL Reference Links

[SDL3](https://wiki.libsdl.org/SDL3/FrontPage)
[SDL_Image Quick Reference](https://wiki.libsdl.org/SDL3_image/QuickReference)
