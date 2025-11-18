#ifndef SOT_GPU_PIPELINE_H_
#define SOT_GPU_PIPELINE_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "sot_gpu_pipeline.h"

SDL_AppResult SOT_InitializeWindow(AppState *as);
SDL_AppResult SOT_InitializePipeline(AppState *as);

#endif

/** Extract from SDL documentation on shader setss.
 * Creates a shader to be used when creating a graphics pipeline.
 *
 * Shader resource bindings must be authored to follow a particular order
 * depending on the shader format.
 *
 * For SPIR-V shaders, use the following resource sets:
 *
 * For vertex shaders:
 *
 * - 0: Sampled textures, followed by storage textures, followed by storage
 *   buffers
 * - 1: Uniform buffers
 *
 * For fragment shaders:
 *
 * - 2: Sampled textures, followed by storage textures, followed by storage
 *   buffers
 * - 3: Uniform buffers
 *
 * For DXBC and DXIL shaders, use the following register order:
 *
 * For vertex shaders:
 *
 * - (t[n], space0): Sampled textures, followed by storage textures, followed
 *   by storage buffers
 * - (s[n], space0): Samplers with indices corresponding to the sampled
 *   textures
 * - (b[n], space1): Uniform buffers
 *
 * For pixel shaders:
 *
 * - (t[n], space2): Sampled textures, followed by storage textures, followed
 *   by storage buffers
 * - (s[n], space2): Samplers with indices corresponding to the sampled
 *   textures
 * - (b[n], space3): Uniform buffers
 *
 * For MSL/metallib, use the following order:
 *
 * - [[texture]]: Sampled textures, followed by storage textures
 * - [[sampler]]: Samplers with indices corresponding to the sampled textures
 * - [[buffer]]: Uniform buffers, followed by storage buffers. Vertex buffer 0
 *   is bound at [[buffer(14)]], vertex buffer 1 at [[buffer(15)]], and so on.
 *   Rather than manually authoring vertex buffer indices, use the
 *   [[stage_in]] attribute which will automatically use the vertex input
 *   information from the SDL_GPUGraphicsPipeline.
 *
 * Shader semantics other than system-value semantics do not matter in D3D12
 * and for ease of use the SDL implementation assumes that non system-value
 * semantics will all be TEXCOORD. If you are using HLSL as the shader source
 * language, your vertex semantics should start at TEXCOORD0 and increment
 * like so: TEXCOORD1, TEXCOORD2, etc. If you wish to change the semantic
 * prefix to something other than TEXCOORD you can use
 * SDL_PROP_GPU_DEVICE_CREATE_D3D12_SEMANTIC_NAME_STRING with
 * SDL_CreateGPUDeviceWithProperties().
 * */