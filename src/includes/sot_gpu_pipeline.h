#ifndef SOT_GPU_PIPELINE_H_
#define SOT_GPU_PIPELINE_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "common.h"

// Forward-declare AppState to break circular dependency
struct AppState;
struct SOT_Scene;

// ---------------------------------- GPU Rendering Creation Data Structures --------------------//

// Enum describing the currently supported pipelines we can use 
// during the render pass.
typedef enum SOT_PipelineID {
    SOT_RP_TEST,
    SOT_RP_TILEMAP,
    SOT_RP_SPRITE,
    SOT_RP_OVERLAY,
} SOT_PipelineID;

typedef enum {
    SOT_RPF_NONE           = 0,
    SOT_RPF_TEST           = 1 << 0,  // 1
    SOT_RPF_SPRITES        = 1 << 1,  // 1
    SOT_RPF_TILEMAP        = 1 << 2,  // 2
    SOT_RPF_OVERLAY        = 1 << 3,   // 4
} SOT_GPU_PIPELINE_FLAGS;

// GPU Buffers Management data structures

typedef enum {
    SOT_BUFFER_NONE         = 0,
    SOT_BUFFER_VERTEX       = 1 << 0,  // 1
    SOT_BUFFER_INDEX        = 1 << 1,  // 2
    SOT_BUFFER_TEXTURE      = 1 << 2,  // 4
    SOT_BUFFER_TILEMAP      = 1 << 3,  // 8
} SOT_GPU_BUFFER_FLAGS;


typedef struct SOT_GPU_Buffers {
    SDL_GPUBuffer *vertexBuffer;
    SDL_GPUBuffer *indexBuffer;
    SDL_GPUBuffer *storageBuffer[8];
    SDL_GPUTexture *textures[16];
    int texturesCount;
    int storageBuffersCount;
} SOT_GPU_Buffers;


typedef struct SOT_GPU_TransferBuffers {
    SDL_GPUTransferBuffer *vertexTransferBuffer;
    SDL_GPUTransferBuffer *indexTransferBuffer;
    SDL_GPUTransferBuffer *textureTransferBuffer;
    SDL_GPUTransferBuffer *storageTransferBuffer;
} SOT_GPU_TransferBuffers;


/// @brief Struct holding information about a shader.
/// Required to load a shader in memory; consumed by SOT_GPU_PipelineInfo.
/// ```text
/// name               : Shader name
/// samplerCount       : Number of texture samplers
/// uniformCount       : Number of uniforms
/// storageBufferCount : Number of storage buffers
/// sorageTextureCount : Number of storage textures
/// ```
typedef struct SOT_GPU_ShaderInfo {
    char *name;              
    int samplerCount;        
    int uniformCount;        
    int storageBufferCount;  
    int sorageTextureCount;  
} SOT_GPU_ShaderInfo;

// Holds data describing the parameters required to initialize a 
// GPU Pipeline
typedef struct SOT_GPU_PipelineInfo {
    SOT_PipelineID pipeline_ID;
    SOT_GPU_ShaderInfo *vertexShader;
    SOT_GPU_ShaderInfo *fragmentShader;
    mat4 pvMatrix;
} SOT_GPU_PipelineInfo;

// CPU bound structure holding the data
// passed to the SOT_GPU_Upload functions to load
// buffers and textures to the GPU VRAM.
typedef struct SOT_GPU_Data {
    vertex *vertexData;
    int vertexDataSize;
    uint16_t *indexData;
    int indexDataSize;
    SDL_Surface *surfaces[16];
    int surfaceCount;
    int *tilemapData;
    int tilemapDataSize;
} SOT_GPU_Data;

typedef struct SOT_GPU_State {
    SOT_PipelineID pipelineID;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GPUDevice *device;
    SDL_GPUSampler *nearestSampler;
    // GPU buffers
    SOT_GPU_Buffers buffers;
    SOT_GPU_TransferBuffers transferBuffers;
    // pipelines array
    uint32_t pipelineFlags;
    SDL_GPUGraphicsPipeline *pipeline[16];
} SOT_GPU_State;

typedef struct SOT_GPU_RenderpassInfo {
    SDL_GPURenderPass *renderpass;
    SDL_GPUTextureSamplerBinding *samplerBindings;
    SDL_GPUCommandBuffer *cmdBuffer;
   
} SOT_GPU_RenderpassInfo;


SDL_AppResult SOT_GPU_InitRenderer(struct AppState *as, uint32_t pipelinesFlags);
SDL_AppResult SOT_GPU_InitPipelineWithInfo(SOT_GPU_State* gpu, SOT_GPU_PipelineInfo *pipelineInfo);

// ------------------------------------------- Buffers Data Management --------------------------------------//
SDL_AppResult SOT_MapVertexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data);
SDL_AppResult SOT_MapIndexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data);
SDL_AppResult SOT_MapTextureData(SOT_GPU_State *gpu, SOT_GPU_Data *data);
SDL_AppResult SOT_MapTilemapData(SOT_GPU_State *gpu, SOT_GPU_Data *data);
SDL_AppResult SOT_UploadVertexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass);
SDL_AppResult SOT_UploadIndexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass);
SDL_AppResult SOT_UploadTextureData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass);
SDL_AppResult SOT_UploadTilemapData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass);
SDL_AppResult SOT_UploadBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data, uint32_t bufferFlags);
SDL_AppResult SOT_GPU_Render(SOT_GPU_State *gpu, struct SOT_Scene *scene);

// ------------------------------------------ Test Methods ---------------------------------------------//
void SOT_GPU_InitializeTestData(SOT_GPU_State *gpu);

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