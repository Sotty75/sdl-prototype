#include "sot_gpu_debug_info.h"
#include "sot_gpu_pipeline.h"


void SOT_GPU_InitializeDebugInfo(SOT_GPU_State *gpu) {

    gpu->debugInfo = (SOT_GPU_DebugInfo *) SDL_malloc(sizeof(SOT_GPU_DebugInfo));
    gpu->debugInfo->linesCount = 0;
    gpu->debugInfo->linesCapacity = 0;
    gpu->debugInfo->vertexList = NULL;
}


void SOT_GPU_AddLine(SOT_GPU_State *gpu, vec3 startPoint, vec3 endPoint)
{
    SOT_GPU_DebugInfo *di = gpu->debugInfo;

    if (di->linesCount >= di->linesCapacity) {
        int newCap = (di->linesCapacity == 0) ? 64 : di->linesCapacity * 2;
        di->vertexList = (vertex *) SDL_realloc(di->vertexList, 2 * newCap * sizeof(vertex));
        di->linesCapacity = newCap;
    }

    int i = di->linesCount;
    di->vertexList[2 * i] = (vertex) {
        {startPoint[0], startPoint[1], startPoint[2]},
        {1.0, 0.0, 0.0},
        {0.0, 0.0}
    };
    di->vertexList[2 * i + 1] = (vertex) {
        {endPoint[0], endPoint[1], endPoint[2]},
        {1.0, 0.0, 0.0},
        {0.0, 0.0}
    };

    di->linesCount++;
}

void SOT_GPU_ClearLines(struct SOT_GPU_State *gpu) {
    if (gpu->debugInfo != NULL) {
        gpu->debugInfo->linesCount = 0;
    }
    else {
        SOT_GPU_InitializeDebugInfo(gpu);
    }
}

void SOT_GPU_UploadDebugInfo(SOT_GPU_State *gpu) {
    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};
    gpuData.pipelineID = SOT_RP_DEBUG;

    // Vertext Buffer Data
    gpuData.vertexDataSize = 2 * (gpu->debugInfo->linesCount) * sizeof(vertex);
    gpuData.vertexData = (vertex *) SDL_malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, gpu->debugInfo->vertexList, gpuData.vertexDataSize);

    //...upload data to GPU buffers used by the shader
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX);

    SDL_free(gpuData.vertexData);
}

void SOT_GPU_RenderDebugInfo(SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix) 
{
    SDL_BindGPUGraphicsPipeline(rpi->renderpass, gpu->pipeline[SOT_RP_DEBUG]);
    SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 0, pvMatrix, sizeof(mat4));
    SDL_BindGPUVertexBuffers(rpi->renderpass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->buffers[SOT_RP_DEBUG].vertexBuffer, .offset = 0}, 1);

    // Draw all the tiles of the shader
    SDL_DrawGPUPrimitives(rpi->renderpass, gpu->debugInfo->linesCount * 2, 1, 0, 0);
}