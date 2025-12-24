#ifndef SOT_GPU_PIPELINE_H_
#define SOT_GPU_PIPELINE_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "common.h"

// Forward-declare AppState to break circular dependency
struct AppState;


// ---------------------------------- Tilemap Render Pipeline Info -----------------------------//

// Tilemap Information loaded as a uniform in the tilemap 
// vertex shader
// 
// - COLUMNS: Tilemap columns
// - TILE_WIDTH: width of a single tile in pixels.
// - TILE_HEIGHT: height of a single tile in pixels.
// - TILESET_WIDTH: width of the source tileset in pixels.
typedef struct SOT_GPU_TilemapInfo {
    int COLUMNS;
    int TILE_WIDTH;
    int TILE_HEIGHT;
    int TILESET_WIDTH;
} SOT_GPU_TilemapInfo;

// TEMP: Data used as input for the tilemap rendering in 
// the main render pass.
typedef struct SOT_GPU_Tilemap {
    char *tilesetName;
    mat4 *transformData;
    int transformDataSize;
} SOT_GPU_Tilemap;

// ---------------------------------- GPU Rendering Creation Data Structures --------------------//

// Enum describing the currently supported pipelines we can use 
// during the render pass.
typedef enum SOT_Pipeline_ID {
    SOT_RP_TILEMAP,
    SOT_RP_SPRITE
} SOT_Pipeline_ID;


// CPU bound structure holding the data
// passed to the SOT_GPU_Upload functions to load
// buffers and textures to the GPU VRAM.
typedef struct SOT_GPU_Data {
    vertex *vertexData;
    uint16_t* indexData;
    SDL_Surface *surfaces[16];
    SOT_GPU_Tilemap *tilemapData;
    int vertexDataSize;
    int indexDataSize;
    int surfaceCount;
} SOT_GPU_Data;

// Struct holding information about a shader
// required to load a shader in memory, consumed by the SOT_GPU_PipelineInfo
typedef struct SOT_GPU_ShaderInfo {
    char *name;
    int samplerCount;
    int uniformCount;
    int storageBufferCount;
    int textureCount;
} SOT_GPU_ShaderInfo;

// Holds data describing the parameters required to initialize a 
// GPU Pipeline
typedef struct SOT_GPU_PipelineInfo {
    SOT_Pipeline_ID pipeline_ID;
    SOT_GPU_ShaderInfo *vertexShader;
    SOT_GPU_ShaderInfo *fragmentShader;
} SOT_GPU_PipelineInfo;

typedef struct SOT_GPU_State {
    SOT_Pipeline_ID pipeline_ID;
    SDL_Renderer *renderer;
    SDL_GPUDevice *device;
    SDL_GPUSampler *nearestSampler;
    SDL_GPUBuffer *vertexBuffer;
    SDL_GPUBuffer *indexBuffer;
    SDL_GPUBuffer *storageBuffer[8];
    SDL_GPUTexture *textures[16];
    int texturesCount;
    int storageBuffersCount;

    // pipelines fields
    SDL_GPUGraphicsPipeline *pipeline[16];

} SOT_GPU_State;

SDL_AppResult SOT_InitializeWindow(struct AppState *as);
SDL_AppResult SOT_InitializePipelineWithInfo(struct AppState *as, SOT_GPU_PipelineInfo *pipelineInfo);
SDL_AppResult SOT_UploadBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data);
SDL_AppResult SOT_UploadTilemap(SOT_GPU_State *gpu, SOT_GPU_Data *data);
SDL_AppResult SOT_Render(struct AppState *as, mat4 projection_view);
#endif

/*
 * Extract from SDL documentation on shader setss.
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
 */