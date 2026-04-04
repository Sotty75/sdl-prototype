#include "sot_display.h"
#include "sot_gpu_pipeline.h"

void SOT_Display_Init(SOT_GPU_State *gpu, int internalW, int internalH)
{
    SOT_Display *display = &gpu->display;
    display->internalWidth = internalW;
    display->internalHeight = internalH;

    // Create the offscreen render target texture at internal resolution
    display->framebuffer = SDL_CreateGPUTexture(gpu->device, &(SDL_GPUTextureCreateInfo) {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width = internalW,
        .height = internalH,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
    });

    if (display->framebuffer == NULL) {
        SDL_Log("Failed to create virtual framebuffer texture: %s", SDL_GetError());
        return;
    }

    // Compute initial scaling based on current window size
    int windowW, windowH;
    SDL_GetWindowSize(gpu->window, &windowW, &windowH);
    SOT_Display_UpdateScaling(display, windowW, windowH);
}

void SOT_Display_UpdateScaling(SOT_Display *display, int windowW, int windowH)
{
    // Compute the largest integer scale that fits both dimensions
    int scaleX = windowW / display->internalWidth;
    int scaleY = windowH / display->internalHeight;
    int scale = (scaleX < scaleY) ? scaleX : scaleY;
    if (scale < 1) scale = 1;

    display->scaleFactor = scale;
    display->viewportW = display->internalWidth * scale;
    display->viewportH = display->internalHeight * scale;
    display->viewportX = (windowW - display->viewportW) / 2;
    display->viewportY = (windowH - display->viewportH) / 2;
}

void SOT_Display_Destroy(SOT_GPU_State *gpu)
{
    if (gpu->display.framebuffer != NULL) {
        SDL_ReleaseGPUTexture(gpu->device, gpu->display.framebuffer);
        gpu->display.framebuffer = NULL;
    }
}
