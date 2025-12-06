
#include "sot_engine.h"

SDL_AppResult SOT_InitializeWindow(AppState *as) {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Initialize the window and renderer entities.
    as->pWindow =  SDL_CreateWindow("Test Creazione Finestra", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (as->pWindow == NULL) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SOT_GPU_State *gpu = SDL_malloc(sizeof(SOT_GPU_State));
    if (gpu == NULL) {
        SDL_Log("Unable to allocate memory for the SOT_GPU_State object.");
        return SDL_APP_FAILURE;
    }
       
    // Set the GPU State fields all to zero.
    SDL_memset(gpu, 0, sizeof(SOT_GPU_State));

    gpu->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, 0, NULL);
    if (gpu->device == NULL) {
        SDL_Log("Couldn't create GPU Device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu->device, as->pWindow)) {
        SDL_Log("Couldn't bind GPU Device to SDL Window: %s", SDL_GetError());
    }

    // Initialize the GPU Device Samplers
    gpu->nearestSampler = SDL_CreateGPUSampler(gpu->device, &(SDL_GPUSamplerCreateInfo) {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	});

    as->gpu = gpu;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_InitializePipelineWithInfo(AppState *as, SOT_GPU_PipelineInfo *info) {

   SOT_GPU_State *gpu = as->gpu;

    // Initialize base path used to load assets
    InitializeAssetsLoader();

    // Create the shaders
	SDL_GPUShader *vertexShader = LoadShader(gpu->device, info->vertexShaderName, 0, 2, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader *fragmentShader = LoadShader(gpu->device, info->fragmentShaderName, 1, 0, 0, 0);
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
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL,
        .rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE,
        .target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(gpu->device, as->pWindow)
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

SDL_AppResult SOT_UploadBufferData(SOT_GPU_State *gpu, SOT_GPU_Data *data) {

    // Vertex Buffer Data Upload

    SDL_GPUBufferCreateInfo vertexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = data->vertexDataSize,
        .props = 0
    };

    gpu->vertexBuffer = SDL_CreateGPUBuffer(gpu->device, &vertexBufferInfo);
    if (gpu->vertexBuffer == NULL)
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
    SDL_GPUTransferBuffer *transferBuffer = SDL_CreateGPUTransferBuffer(gpu->device, &transferBufferInfo);
    if (transferBuffer == NULL)
	{
		SDL_Log("Failed to create transfer buffer!");
		return -SDL_APP_FAILURE;
	}

    vertex* transferData = SDL_MapGPUTransferBuffer(gpu->device, transferBuffer, false);
    SDL_memcpy(transferData,  data->vertexData, data->vertexDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, transferBuffer);

    // ------------------------------ Index Data Buffer - START ------------------------------------------//
    
    SDL_GPUBufferCreateInfo indexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = data->indexDataSize,
        .props = 0
    };

    gpu->indexBuffer = SDL_CreateGPUBuffer(gpu->device, &indexBufferInfo);
    if (gpu->indexBuffer == NULL)
	{
		SDL_Log("Failed to create index buffer!");
		return SDL_APP_FAILURE;
	}


    SDL_GPUTransferBuffer *indexTransferBuffer = SDL_CreateGPUTransferBuffer
    (gpu->device, 
        &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = data->indexDataSize,
        .props = 0
    });
    
    if (indexTransferBuffer == NULL)
	{
		SDL_Log("Failed to create transfer buffer!");
		return SDL_APP_FAILURE;
	}

    uint16_t* transferIndexData = SDL_MapGPUTransferBuffer(gpu->device, indexTransferBuffer, false);
    SDL_memcpy(transferIndexData, data->indexData, data->indexDataSize);
    SDL_UnmapGPUTransferBuffer(gpu->device, indexTransferBuffer);

     // ------------------------------ Texture Data Buffer ------------------------------------------//
	
    int textSize = 0;
    for (int i = 0; i < data->surfaceCount; ++i)
        textSize += (data->surfaces[i]->w * data->surfaces[i]->h * 4);

    SDL_GPUTransferBufferCreateInfo textureTransferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = textSize
	};
    
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(gpu->device, &textureTransferBufferInfo);
    if (textureTransferBuffer == NULL)
	{
		SDL_Log("Failed to create texture transfer buffer!");
		return -SDL_APP_FAILURE;
	}
    
    Uint8* textureData = SDL_MapGPUTransferBuffer(gpu->device, textureTransferBuffer,	false);
    Uint8* textureDataTemp = textureData;
    for (int i = 0; i < data->surfaceCount; ++i) {
	    SDL_memcpy(textureDataTemp, data->surfaces[i]->pixels, data->surfaces[i]->w * data->surfaces[i]->h * 4);
        textureDataTemp +=  data->surfaces[i]->w * data->surfaces[i]->h * 4;
    }
	SDL_UnmapGPUTransferBuffer(gpu->device, textureTransferBuffer);

    //---------------------------------------- Upload Vertex/Index/Textures Buffers to the GPU ------------------------------//

	// Upload the transfer data to the vertex buffer
	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (uploadCmdBuf == NULL)
	{
		SDL_Log("Failed to acquire command buffer!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

	SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = gpu->vertexBuffer,
			.offset = 0,
			.size =  data->vertexDataSize
		},
		false
	);

    SDL_UploadToGPUBuffer(
		copyPass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = indexTransferBuffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = gpu->indexBuffer,
			.offset = 0,
			.size =  data->indexDataSize
		},
		false
	);

    for (int i = 0; i < data->surfaceCount; ++i) {

        int offset = 0;
        for (int j = 0; j < i; j++) {
            if (j == 0) continue;
            offset += data->surfaces[j]->w * data->surfaces[j]->h * 4;
        }
    
        gpu->textures[i] = SDL_CreateGPUTexture(gpu->device, &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .width = data->surfaces[i]->w,
            .height = data->surfaces[i]->h,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
    	});
        gpu->texturesCount++;


        SDL_UploadToGPUTexture(
            copyPass,
            &(SDL_GPUTextureTransferInfo) {
                .transfer_buffer = textureTransferBuffer,
                .offset = offset /* Zeros out the rest */
            },
            &(SDL_GPUTextureRegion){
                .texture = gpu->textures[i],
                .w = data->surfaces[i]->w,
                .h = data->surfaces[i]->h,
                .d = 1
            },
            false
	    );
    }
    
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
	SDL_ReleaseGPUTransferBuffer(gpu->device, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(gpu->device, indexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(gpu->device, textureTransferBuffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SOT_RenderScene(struct AppState *as, mat4 transforms[], int count, mat4 projection_view) {
    SOT_GPU_State *gpu = as->gpu;
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (cmdbuf == NULL)
    {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, as->pWindow, &swapchainTexture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
	colorTargetInfo.texture = swapchainTexture;
	colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
	colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUTextureSamplerBinding textureBindings[gpu->texturesCount];

    for (int i = 0; i < gpu->texturesCount; i++ ) {
        textureBindings[i] = (SDL_GPUTextureSamplerBinding) {
            .texture = gpu->textures[i], 
            .sampler = gpu->nearestSampler
        };
    }

	if (swapchainTexture != NULL)
	{

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			cmdbuf,
			&colorTargetInfo,
			1,
			NULL
		);

        SDL_BindGPUGraphicsPipeline(renderPass, gpu->pipeline[SOT_RP_TILEMAP]);
		SDL_BindGPUFragmentSamplers(renderPass, 0, textureBindings, 2);
	    SDL_PushGPUVertexUniformData(cmdbuf, 0, transforms, count * sizeof(mat4));
        SDL_PushGPUVertexUniformData(cmdbuf, 1, projection_view, sizeof(mat4));
        SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding) { .buffer = gpu->vertexBuffer, .offset = 0}, 1);
        SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding) {.buffer = gpu->indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(renderPass, 6, 9, 0, 0, 0);
		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);
    return SDL_APP_CONTINUE;  
}