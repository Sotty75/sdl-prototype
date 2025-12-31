#include "sot_gpu_debug_info.h"
#include "sot_gpu_pipeline.h"


void SOT_GPU_InitializeDebugInfo(SOT_GPU_State *gpu) {

    // allocate the debug info object memory
    gpu->debugInfo = (SOT_GPU_DebugInfo *) malloc(sizeof(SOT_GPU_DebugInfo));
    gpu->debugInfo->linesCount = 0;
    gpu->debugInfo->vertexList = NULL;

    return;
}


void SOT_GPU_AddLine(SOT_GPU_State *gpu, vec3 startPoint, vec3 endPoint) 
{
    int linesCount = gpu->debugInfo->linesCount;
    vertex *vertexList = gpu->debugInfo->vertexList;
     
    if (linesCount == 0) {
        vertexList = (vertex*) SDL_malloc(2 * sizeof(vertex));
        vertexList[0] = (vertex) {
            {startPoint[0], startPoint[1], startPoint[2]},
            {1.0, 0.0, 0.0}, 
            {0.0, 0.0}
        };
        vertexList[1] = (vertex) {
            {endPoint[0], endPoint[1], endPoint[2]},
            {1.0, 0.0, 0.0}, 
            {0.0, 0.0}
        };
        gpu->debugInfo->vertexList = vertexList;
    }
    else {
        vertex *newList = (vertex*) SDL_malloc(2 * (linesCount + 1) * sizeof(vertex));
        SDL_memcpy(newList, vertexList, 2 * (linesCount) * sizeof(vertex));
        newList[2*linesCount] = (vertex) {
            {startPoint[0], startPoint[1], startPoint[2]},
            {1.0, 0.0, 0.0}, 
            {0.0, 0.0}
        };
        newList[2*linesCount + 1] = (vertex) {
            {endPoint[0], endPoint[1], endPoint[2]},
            {1.0, 0.0, 0.0}, 
            {0.0, 0.0}
        };
        SDL_free(vertexList);
        gpu->debugInfo->vertexList = newList;
    }

    gpu->debugInfo->linesCount++;
    return;
}

void SOT_GPU_ClearLines(struct SOT_GPU_State *gpu) {
    if (gpu->debugInfo != NULL) {
        gpu->debugInfo->linesCount = 0;
        
        if (gpu->debugInfo->vertexList != NULL)
            SDL_free(gpu->debugInfo->vertexList);

        gpu->debugInfo->vertexList = NULL;    
    }
    else {
        SOT_GPU_InitializeDebugInfo(gpu);
    }
    
    return;
}

void SOT_GPU_RefreshDebugInfo(SOT_GPU_State *gpu) {
    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};
    gpuData.pipelineID = SOT_RP_DEBUG;

    // Vertext Buffer Data
    gpuData.vertexDataSize = 2 * (gpu->debugInfo->linesCount) * sizeof(vertex);
    gpuData.vertexData = (vertex *) malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, gpu->debugInfo->vertexList, gpuData.vertexDataSize);

    //...upload data to GPU buffers used by the shader
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX);
}

void SOT_GPU_RenderDebugInfo(SOT_GPU_State *gpu, SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix) 
{
    SDL_BindGPUGraphicsPipeline(rpi->renderpass, gpu->pipeline[SOT_RP_DEBUG]);
    SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 0, pvMatrix, sizeof(mat4));
    SDL_BindGPUVertexBuffers(rpi->renderpass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->buffers[SOT_RP_DEBUG].vertexBuffer, .offset = 0}, 1);

    // Draw all the tiles of the shader
    SDL_DrawGPUPrimitives(rpi->renderpass, gpu->debugInfo->linesCount * 2, 1, 0, 0);
}