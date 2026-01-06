
#include "sot_engine.h"


SDL_AppResult SOT_GPU_InitRenderer(struct AppState *as, uint32_t pipelinesFlags) {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // GPU State object allocation
    SOT_GPU_State *gpu = SDL_malloc(sizeof(SOT_GPU_State));
    if (gpu == NULL) {
        SDL_Log("Unable to allocate memory for the SOT_GPU_State object.");
        return SDL_APP_FAILURE;
    }
       
    // Set the GPU State fields all to zero.
    SDL_memset(gpu, 0, sizeof(SOT_GPU_State));


    // Initialize the window and renderer entities.
    gpu->window =  SDL_CreateWindow("SDL GPU Prototype", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (gpu->window == NULL) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    gpu->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, 0, NULL);
    if (gpu->device == NULL) {
        SDL_Log("Couldn't create GPU Device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu->device, gpu->window)) {
        SDL_Log("Couldn't bind GPU Device to SDL Window: %s", SDL_GetError());
    }

    // Initialize the GPU Device Samplers
    gpu->nearestSampler = SDL_CreateGPUSampler(gpu->device, &(SDL_GPUSamplerCreateInfo) {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	});

    // Initialize the rendering pipelines
    if (pipelinesFlags & SOT_RPF_TEST) 
        SOT_GPU_InitPipelineWithInfo(gpu, &(SOT_GPU_PipelineInfo) {
           .pipeline_ID = SOT_RP_TEST,
           .vertexShader = &(SOT_GPU_ShaderInfo) {"shaderTexture.vert", 0, 1, 0, 0},
           .fragmentShader = &(SOT_GPU_ShaderInfo) {"shaderTexture.frag", 1, 0, 0, 0},
           .primitiveType =SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });

    if (pipelinesFlags & SOT_RPF_TILEMAP) 
        SOT_GPU_InitPipelineWithInfo(gpu, &(SOT_GPU_PipelineInfo) {
           .pipeline_ID = SOT_RP_TILEMAP,
           .vertexShader = &(SOT_GPU_ShaderInfo) {"shaderTilemap.vert", 0, 2, 1, 0},
           .fragmentShader = &(SOT_GPU_ShaderInfo) {"shaderTilemap.frag", 1, 0, 0, 0},
           .primitiveType =SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });

    if (pipelinesFlags & SOT_RPF_SPRITES) 
        SOT_GPU_InitPipelineWithInfo(gpu, &(SOT_GPU_PipelineInfo) {
           .pipeline_ID = SOT_RP_SPRITE,
           .vertexShader = &(SOT_GPU_ShaderInfo) {"shaderSprite.vert", 0, 1, 1, 0},
           .fragmentShader = &(SOT_GPU_ShaderInfo) {"shaderSprite.frag", 1, 0, 0, 0},
           .primitiveType =SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });

    if (pipelinesFlags & SOT_RPF_OVERLAY) 
        SOT_GPU_InitPipelineWithInfo(gpu, &(SOT_GPU_PipelineInfo) {
           .pipeline_ID = SOT_RP_TEST,
           .vertexShader = &(SOT_GPU_ShaderInfo) {"shaderTexture.vert", 0, 1, 0, 0},
           .fragmentShader = &(SOT_GPU_ShaderInfo) {"shaderTexture.frag", 1, 0, 0, 0},
           .primitiveType =SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
    });

    if (pipelinesFlags & SOT_RPF_DEBUG) 
        SOT_GPU_InitPipelineWithInfo(gpu, &(SOT_GPU_PipelineInfo) {
           .pipeline_ID = SOT_RP_DEBUG,
           .vertexShader = &(SOT_GPU_ShaderInfo) {"shaderDebug.vert", 0, 1, 0, 0},
           .fragmentShader = &(SOT_GPU_ShaderInfo) {"shaderDebug.frag", 0, 0, 0, 0},
           .primitiveType =SDL_GPU_PRIMITIVETYPE_LINELIST,
    });


    gpu->pipelineFlags = pipelinesFlags;
    as->gpu = gpu;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_GPU_InitPipelineWithInfo(SOT_GPU_State *gpu, SOT_GPU_PipelineInfo *info) {
    // Initialize base path used to load assets
    InitializeAssetsLoader();

    // Create the shaders
	SDL_GPUShader *vertexShader = LoadShader(gpu->device, 
            info->vertexShader->name,
            info->vertexShader->samplerCount, 
            info->vertexShader->uniformCount, 
            info->vertexShader->storageBufferCount, 
            info->vertexShader->sorageTextureCount);

	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader *fragmentShader = LoadShader(gpu->device, 
        info->fragmentShader->name,
        info->fragmentShader->samplerCount, 
        info->fragmentShader->uniformCount, 
        info->fragmentShader->storageBufferCount, 
        info->fragmentShader->sorageTextureCount);

	if (fragmentShader == NULL)	{
		SDL_Log("Failed to create fragment shader!");
		return SDL_APP_FAILURE;
	}

    /* Descriptor for the vertex buffer. 
     * An array of vertex buffers descriptors (one per each vertex buffer we want in the pipeline)
     */ 
    SDL_GPUVertexBufferDescription buffersDescription[] = {
        {
            .slot = 0,
            .pitch = sizeof(vertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        }, 
    };
    
    /* Descriptor for the vertex attributes structure, it is an array of vertex attributes */
    SDL_GPUVertexAttribute attributesDesc[] = {
        //...position attribute
        {
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .location = 0,
            .offset = 0
        },
        //...color attribute
        {
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .location = 1,
            .offset = sizeof(vec3)
        },
        //...texture attribute
        {
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .location = 2,
            .offset = sizeof(vec3) + sizeof(vec3)
        }
    };

    /* Previous two descriptors are used int he GPU Vertex Input State structure 
     * required by the pipeline */
    SDL_GPUVertexInputState vertexInputState = {
        .vertex_buffer_descriptions = buffersDescription,
        .num_vertex_buffers = 1,
        .vertex_attributes = attributesDesc,
        .num_vertex_attributes = 3,
    };

     /* Info structure to be passed to the GPU pipeline creation function */
	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
        .vertex_input_state = vertexInputState,
        .primitive_type = info->primitiveType,
        .rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL,
        .rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE,
        .target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(gpu->device, gpu->window)
			}},
		},
	};

    SDL_GPUGraphicsPipeline *renderingPipeline = SDL_CreateGPUGraphicsPipeline(gpu->device, &pipelineCreateInfo);
	if (renderingPipeline == NULL)
	{
		SDL_Log("Failed to create fill pipeline!");
		return SDL_APP_FAILURE;
	}

    gpu->pipeline[info->pipeline_ID] = renderingPipeline;
    
    // Clean up shader resources
	SDL_ReleaseGPUShader(gpu->device, vertexShader);
	SDL_ReleaseGPUShader(gpu->device, fragmentShader);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_MapVertexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data) {

    // ------------------------------ Vertex Data Buffer - START ------------------------------------------//

    if (gpu->buffers[data->pipelineID].vertexBuffer != NULL) {
        SDL_ReleaseGPUBuffer(gpu->device, gpu->buffers[data->pipelineID].vertexBuffer);
        gpu->buffers[data->pipelineID].vertexBuffer = NULL;
    }

    SDL_GPUBufferCreateInfo vertexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = data->vertexDataSize,
        .props = 0
    };

    gpu->buffers[data->pipelineID].vertexBuffer = SDL_CreateGPUBuffer(gpu->device, &vertexBufferInfo);
    if (gpu->buffers[data->pipelineID].vertexBuffer == NULL)
	{
		SDL_Log("Failed to create vertex buffer!");
		return SDL_APP_FAILURE;
	}

    SDL_GPUTransferBufferCreateInfo transferBufferInfo = 
    {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size =  data->vertexDataSize,
        .props = 0
    };
    gpu->transferBuffers.vertexTransferBuffer = SDL_CreateGPUTransferBuffer(gpu->device, &transferBufferInfo);
    if (gpu->transferBuffers.vertexTransferBuffer == NULL)
	{
		SDL_Log("Failed to create transfer buffer!");
		return SDL_APP_FAILURE;
	}

    vertex* transferData = SDL_MapGPUTransferBuffer(gpu->device, gpu->transferBuffers.vertexTransferBuffer, false);
    SDL_memcpy(transferData,  data->vertexData, data->vertexDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, gpu->transferBuffers.vertexTransferBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_MapIndexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data) {

    // ------------------------------ Index Data Buffer - START ------------------------------------------//
    
    if (gpu->buffers[data->pipelineID].indexBuffer != NULL) {
        SDL_ReleaseGPUBuffer(gpu->device, gpu->buffers[data->pipelineID].indexBuffer);
        gpu->buffers[data->pipelineID].indexBuffer = NULL;
    }

    SDL_GPUBufferCreateInfo indexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = data->indexDataSize,
        .props = 0
    };

    gpu->buffers[data->pipelineID].indexBuffer = SDL_CreateGPUBuffer(gpu->device, &indexBufferInfo);
    if (gpu->buffers[data->pipelineID].indexBuffer == NULL)
	{
		SDL_Log("Failed to create index buffer!");
		return SDL_APP_FAILURE;
	}


    gpu->transferBuffers.indexTransferBuffer = SDL_CreateGPUTransferBuffer
    (gpu->device, 
        &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = data->indexDataSize,
        .props = 0
    });
    
    if (gpu->transferBuffers.indexTransferBuffer == NULL)
	{
		SDL_Log("Failed to create transfer buffer!");
		return SDL_APP_FAILURE;
	}

    uint16_t* transferIndexData = SDL_MapGPUTransferBuffer(gpu->device, gpu->transferBuffers.indexTransferBuffer, false);
    SDL_memcpy(transferIndexData, data->indexData, data->indexDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, gpu->transferBuffers.indexTransferBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_MapTilemapData(SOT_GPU_State *gpu, SOT_GPU_Data *data) {
   
    if (gpu->buffers[data->pipelineID].storageBuffer[0] != NULL) {
        SDL_ReleaseGPUBuffer(gpu->device, gpu->buffers[data->pipelineID].storageBuffer[0]);
        gpu->buffers[data->pipelineID].storageBuffer[0] = NULL;
    }
    

    SDL_GPUBufferCreateInfo storageBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = data->tilemapDataSize,
        .props = 0
    };

    gpu->buffers[data->pipelineID].storageBuffer[0] = SDL_CreateGPUBuffer(gpu->device, &storageBufferInfo);
    if (gpu->buffers[data->pipelineID].storageBuffer[0] == NULL)
	{
		SDL_Log("Failed to create tilemap storage buffer!");
		return SDL_APP_FAILURE;
	}

    gpu->transferBuffers.storageTransferBuffer = SDL_CreateGPUTransferBuffer
    (gpu->device, 
        &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = data->tilemapDataSize,
        .props = 0
    });

    int* transferData = SDL_MapGPUTransferBuffer(gpu->device, gpu->transferBuffers.storageTransferBuffer, false);
    SDL_memcpy(transferData,  data->tilemapData, data->tilemapDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, gpu->transferBuffers.storageTransferBuffer);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_MapSpriteInfoData(SOT_GPU_State *gpu, SOT_GPU_Data *data) {
   
    int spriteDataSize = 2000 * sizeof(SOT_GPU_SpriteInfo);

    if (gpu->buffers[data->pipelineID].storageBuffer[0] == NULL) {
        
        SDL_GPUBufferCreateInfo storageBufferInfo = {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = spriteDataSize,
            .props = 0
        };

        gpu->buffers[data->pipelineID].storageBuffer[0] = SDL_CreateGPUBuffer(gpu->device, &storageBufferInfo);
        if (gpu->buffers[data->pipelineID].storageBuffer[0] == NULL)
        {
            SDL_Log("Failed to create spriteinfo storage buffer!");
            return SDL_APP_FAILURE;
        }    
    }

    gpu->transferBuffers.storageTransferBuffer = SDL_CreateGPUTransferBuffer
    (gpu->device, 
        &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = spriteDataSize,
        .props = 0
    });

    int* transferData = SDL_MapGPUTransferBuffer(gpu->device, gpu->transferBuffers.storageTransferBuffer, false);
    SDL_memcpy(transferData,  data->sprites, spriteDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, gpu->transferBuffers.storageTransferBuffer);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_MapTextureData(SOT_GPU_State *gpu, SOT_GPU_Data *data) {
       // ------------------------------ Texture Data Buffer ------------------------------------------//
	
    int textSize = 0;
    for (int i = 0; i < data->surfaceCount; ++i)
        textSize += (data->surfaces[i]->w * data->surfaces[i]->h * 4);

    SDL_GPUTransferBufferCreateInfo textureTransferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = textSize
	};
    
    gpu->transferBuffers.textureTransferBuffer = SDL_CreateGPUTransferBuffer(gpu->device, &textureTransferBufferInfo);
    if (gpu->transferBuffers.textureTransferBuffer == NULL)
	{
		SDL_Log("Failed to create texture transfer buffer!");
		return SDL_APP_FAILURE;
	}
    
    Uint8* textureData = SDL_MapGPUTransferBuffer(gpu->device, gpu->transferBuffers.textureTransferBuffer,	false);
    Uint8* textureDataTemp = textureData;
    for (int i = 0; i < data->surfaceCount; ++i) {
	    SDL_memcpy(textureDataTemp, data->surfaces[i]->pixels, data->surfaces[i]->w * data->surfaces[i]->h * 4);
        textureDataTemp +=  data->surfaces[i]->w * data->surfaces[i]->h * 4;
    }
	SDL_UnmapGPUTransferBuffer(gpu->device, gpu->transferBuffers.textureTransferBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_UploadVertexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass)
{
    	SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = gpu->transferBuffers.vertexTransferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = gpu->buffers[data->pipelineID].vertexBuffer,
			.offset = 0,
			.size =  data->vertexDataSize
		},
		false
	);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_UploadIndexBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass)
{
        SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = gpu->transferBuffers.indexTransferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = gpu->buffers[data->pipelineID].indexBuffer,
			.offset = 0,
			.size =  data->indexDataSize
		},
		false
	);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_UploadTextureData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass)
{
    for (int i = 0; i < data->surfaceCount; ++i) {

        int offset = 0;
        for (int j = 0; j < i; j++) {
            if (j == 0) continue;
            offset += data->surfaces[j]->w * data->surfaces[j]->h * 4;
        }
    
        gpu->buffers[data->pipelineID].textures[i] = SDL_CreateGPUTexture(gpu->device, &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .width = data->surfaces[i]->w,
            .height = data->surfaces[i]->h,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
    	});
        gpu->buffers[data->pipelineID].texturesCount++;

        SDL_UploadToGPUTexture(
            copyPass,
            &(SDL_GPUTextureTransferInfo) {
                .transfer_buffer = gpu->transferBuffers.textureTransferBuffer,
                .offset = offset /* Zeros out the rest */
            },
            &(SDL_GPUTextureRegion){
                .texture = gpu->buffers[data->pipelineID].textures[i],
                .w = data->surfaces[i]->w,
                .h = data->surfaces[i]->h,
                .d = 1
            },
            false
	    );
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_UploadTilemapData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass)
{
        SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = gpu->transferBuffers.storageTransferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = gpu->buffers[data->pipelineID].storageBuffer[0],
			.offset = 0,
			.size =  data->tilemapDataSize
		},
		false
	);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_UploadSpriteInfoData(SOT_GPU_State *gpu, SOT_GPU_Data *data, SDL_GPUCopyPass *copyPass)
{
    int spriteDataSize = 2000 * sizeof(SOT_GPU_SpriteInfo);

    SDL_UploadToGPUBuffer(
        copyPass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = gpu->transferBuffers.storageTransferBuffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = gpu->buffers[data->pipelineID].storageBuffer[0],
            .offset = 0,
            .size =  spriteDataSize
        },
        false
	);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_UploadBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data, uint32_t bufferFlags) {

    if (bufferFlags & SOT_BUFFER_VERTEX) {
        SOT_MapVertexBufferData(gpu, data);
    }

    if (bufferFlags & SOT_BUFFER_INDEX) {
        SOT_MapIndexBufferData(gpu, data);
    }

    if (bufferFlags & SOT_BUFFER_TEXTURE) {
        SOT_MapTextureData(gpu, data);
    }

    if (bufferFlags & SOT_TILEMAP_SSB) {
        SOT_MapTilemapData(gpu, data);
    }

    if (bufferFlags & SOT_SPRITES_SSB) {
        SOT_MapSpriteInfoData(gpu, data);
    }

	// Upload the transfer data to the vertex buffer
	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (uploadCmdBuf == NULL)
	{
		SDL_Log("Failed to acquire command buffer!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    if (bufferFlags & SOT_BUFFER_VERTEX) {
        SOT_UploadVertexBufferData(gpu, data, copyPass);
    }

    if (bufferFlags & SOT_BUFFER_INDEX) {
        SOT_UploadIndexBufferData(gpu, data, copyPass);
    }

    if (bufferFlags & SOT_BUFFER_TEXTURE) {
        SOT_UploadTextureData(gpu, data, copyPass);
    }

    if (bufferFlags & SOT_TILEMAP_SSB) {
        SOT_UploadTilemapData(gpu, data, copyPass);
    }
    
    if (bufferFlags & SOT_SPRITES_SSB) {
        SOT_UploasSpriteInfoData(gpu, data);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
    
    // Release the Transfer Buffers
    if (bufferFlags & SOT_BUFFER_VERTEX) {
	    SDL_ReleaseGPUTransferBuffer(gpu->device, gpu->transferBuffers.vertexTransferBuffer);
    }

    if (bufferFlags & SOT_BUFFER_INDEX) {
        SDL_ReleaseGPUTransferBuffer(gpu->device, gpu->transferBuffers.indexTransferBuffer);
    }

    if (bufferFlags & SOT_BUFFER_TEXTURE) {
        SDL_ReleaseGPUTransferBuffer(gpu->device, gpu->transferBuffers.textureTransferBuffer);
    }

    if ((bufferFlags & SOT_TILEMAP_SSB) || (bufferFlags & SOT_SPRITES_SSB)) {
        SDL_ReleaseGPUTransferBuffer(gpu->device, gpu->transferBuffers.storageTransferBuffer);
    }

    return SDL_APP_CONTINUE;
}

/* Renders a scene defied as:
- Tilemap
- Actors
- GUI elements
*/
SDL_AppResult SOT_GPU_Render(SOT_GPU_State *gpu, SOT_Scene *scene) 
{
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (cmdbuf == NULL)
    {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, gpu->window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
	colorTargetInfo.texture = swapchainTexture;
	colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
	colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;


	if (swapchainTexture != NULL)
	{
        // Create the render pass object
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			cmdbuf,
			&colorTargetInfo,
			1,
			NULL
		);

        // Initialize the render pass info structure
        SOT_GPU_RenderpassInfo *rpi = &(SOT_GPU_RenderpassInfo) {
            .cmdBuffer = cmdbuf,
            .renderpass = renderPass,
        };

        if (gpu->pipelineFlags & SOT_RPF_TEST)
        {
            
            SDL_GPUTextureSamplerBinding textureBindings[gpu->buffers[SOT_RP_TEST].texturesCount];
            for (int i = 0; i < gpu->buffers[SOT_RP_TEST].texturesCount; i++ ) {
                textureBindings[i] = (SDL_GPUTextureSamplerBinding) {
                    .texture = gpu->buffers[SOT_RP_TEST].textures[i], 
                    .sampler = gpu->nearestSampler
                };
            }
            SDL_BindGPUGraphicsPipeline(rpi->renderpass, gpu->pipeline[SOT_RP_TEST]);
            SDL_BindGPUFragmentSamplers(rpi->renderpass, 0, textureBindings, gpu->buffers[SOT_RPF_TEST].texturesCount);
            SDL_PushGPUVertexUniformData(rpi->cmdBuffer, 0, scene->worldCamera.pvMatrix, sizeof(mat4));
            SDL_BindGPUVertexBuffers(rpi->renderpass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->buffers[SOT_RPF_TEST].vertexBuffer, .offset = 0}, 1);
            SDL_BindGPUIndexBuffer(rpi->renderpass, &(SDL_GPUBufferBinding) {.buffer = gpu->buffers[SOT_RPF_TEST].indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            // Draw all the tiles of the shader
            SDL_DrawGPUIndexedPrimitives(rpi->renderpass, 6, 1, 0, 0, 0);
        }

        if (gpu->pipelineFlags & SOT_RPF_TILEMAP)
        {
            SOT_GPU_RenderScene(scene, gpu, rpi);
        }
        
        SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);
    return SDL_APP_CONTINUE;  
}

void SOT_GPU_InitializeTestData(SOT_GPU_State *gpu) {

    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};

    // Create a new quad to use as a template for the single tile
    sot_quad quad = sot_quad_create();

    // Vertext Buffer Data
    gpuData.vertexDataSize = QUAD_VERTS * sizeof(vertex);
    gpuData.vertexData = (vertex *) malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, quad.verts, gpuData.vertexDataSize);

    // Index Buffer Data
    gpuData.indexDataSize = QUAD_INDEXES * sizeof(uint16_t);
    gpuData.indexData = (uint16_t *) malloc(gpuData.indexDataSize);
    memcpy(gpuData.indexData, quad.indexes, gpuData.indexDataSize);

    // Textures Data
    SDL_Surface *surface = NULL;
    GetSurfaceFromImage(&surface, "textures", "wall.png");
    gpuData.surfaces[0] = surface;
    gpuData.surfaceCount = 1;

    //...upload data to GPU buffers used by the shader
    SOT_MapVertexBufferData(gpu, &gpuData);
    SOT_MapIndexBufferData(gpu, &gpuData);
    SOT_MapTextureData(gpu, &gpuData);
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX | SOT_BUFFER_INDEX | SOT_BUFFER_TEXTURE);
}

void SOT_GPU_InitializeDebugData(SOT_GPU_State *gpu) {

    // Create a new GPU Data structure
    SOT_GPU_Data gpuData = {0};

    // Create a new quad to use as a template for the single tile
    sot_quad quad = sot_quad_create();

    // Vertext Buffer Data
    gpuData.vertexDataSize = QUAD_VERTS * sizeof(vertex);
    gpuData.vertexData = (vertex *) malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, quad.verts, gpuData.vertexDataSize);

    //...upload data to GPU buffers used by the shader
    SOT_MapVertexBufferData(gpu, &gpuData);
    SOT_UploadBufferData(gpu, &gpuData, SOT_BUFFER_VERTEX);
}