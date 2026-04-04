#ifndef SOT_DISPLAY_H_
#define SOT_DISPLAY_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

// Default internal (virtual) resolution — the retro framebuffer size.
// All game rendering targets this resolution; it is integer-scaled to the window.
#define SOT_INTERNAL_WIDTH  320
#define SOT_INTERNAL_HEIGHT 240

// Default OS window size
#define SOT_WINDOW_WIDTH  1280
#define SOT_WINDOW_HEIGHT 960

// Forward declarations
struct SOT_GPU_State;

typedef struct SOT_Display {
    // The offscreen render target texture (virtual framebuffer)
    SDL_GPUTexture *framebuffer;

    // Internal (game) resolution in pixels
    int internalWidth;
    int internalHeight;

    // Computed at init and on window resize:
    int scaleFactor;    // Integer scale multiplier
    int viewportX;      // Letterbox offset X (left black bar width)
    int viewportY;      // Letterbox offset Y (top black bar height)
    int viewportW;      // Scaled framebuffer width on screen
    int viewportH;      // Scaled framebuffer height on screen
} SOT_Display;

// Initialize the display system: create framebuffer texture, compute scaling.
// Must be called after the GPU device and window are created.
void SOT_Display_Init(struct SOT_GPU_State *gpu, int internalW, int internalH);

// Recompute integer scale factor and letterbox offsets.
// Call on window resize.
void SOT_Display_UpdateScaling(SOT_Display *display, int windowW, int windowH);

// Release GPU resources owned by the display system.
void SOT_Display_Destroy(struct SOT_GPU_State *gpu);

#endif
