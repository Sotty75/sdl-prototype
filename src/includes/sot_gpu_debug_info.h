#ifndef _DEBUG_INFO_H_
#define _DEBUG_INFO_H_

#include "common.h"

struct SOT_GPU_State;
struct SOT_GPU_RenderpassInfo;

typedef struct SOT_GPU_DebugInfo {
    vertex *vertexList;
    int linesCount;
} SOT_GPU_DebugInfo;

void SOT_GPU_InitializeDebugInfo(struct SOT_GPU_State *gpu);
void SOT_GPU_AddLine(struct SOT_GPU_State *gpu, vec3 startPoint, vec3 endPoint);
void SOT_GPU_RefreshDebugInfo(struct SOT_GPU_State *gpu);
void SOT_GPU_RenderDebugInfo(struct SOT_GPU_State *gpu, struct SOT_GPU_RenderpassInfo *rpi, mat4 pvMatrix);
void SOT_GPU_ClearLines(struct SOT_GPU_State *gpu);

#endif
