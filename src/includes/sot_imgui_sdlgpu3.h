/*
 * C declarations for the Dear ImGui SDL GPU3 backend functions.
 *
 * These functions are compiled with extern "C" linkage in imgui_impl_sdlgpu3.cpp
 * (part of the cimgui static library). This header provides C-compatible declarations
 * so our C11 editor code can call them.
 */
#ifndef SOT_IMGUI_SDLGPU3_H_
#define SOT_IMGUI_SDLGPU3_H_

#include <SDL3/SDL_gpu.h>

// C-compatible mirror of ImGui_ImplSDLGPU3_InitInfo (layout must match the C++ struct)
typedef struct ImGui_ImplSDLGPU3_InitInfo {
    SDL_GPUDevice              *Device;
    SDL_GPUTextureFormat        ColorTargetFormat;
    SDL_GPUSampleCount          MSAASamples;
    SDL_GPUSwapchainComposition SwapchainComposition;
    SDL_GPUPresentMode          PresentMode;
} ImGui_ImplSDLGPU3_InitInfo;

// C-compatible mirror of ImGui_ImplSDLGPU3_RenderState (layout must match C++ struct)
typedef struct ImGui_ImplSDLGPU3_RenderState {
    SDL_GPUDevice   *Device;
    SDL_GPUSampler  *SamplerLinear;
    SDL_GPUSampler  *SamplerNearest;
    SDL_GPUSampler  *SamplerCurrent;
} ImGui_ImplSDLGPU3_RenderState;

// Forward-declare ImDrawData (defined by cimgui.h when CIMGUI_DEFINE_ENUMS_AND_STRUCTS is set)
struct ImDrawData;

#ifdef __cplusplus
extern "C" {
#endif

bool  ImGui_ImplSDLGPU3_Init(ImGui_ImplSDLGPU3_InitInfo *info);
void  ImGui_ImplSDLGPU3_Shutdown(void);
void  ImGui_ImplSDLGPU3_NewFrame(void);
void  ImGui_ImplSDLGPU3_PrepareDrawData(struct ImDrawData *draw_data, SDL_GPUCommandBuffer *command_buffer);
void  ImGui_ImplSDLGPU3_RenderDrawData(struct ImDrawData *draw_data, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, SDL_GPUGraphicsPipeline *pipeline);
void  ImGui_ImplSDLGPU3_CreateDeviceObjects(void);
void  ImGui_ImplSDLGPU3_DestroyDeviceObjects(void);

#ifdef __cplusplus
}
#endif

#endif
