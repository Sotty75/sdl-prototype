
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

    as->gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, 0, NULL);
    if (as->gpuDevice == NULL) {
        SDL_Log("Couldn't create GPU Device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(as->gpuDevice, as->pWindow)) {
        SDL_Log("Couldn't bind GPU Device to SDL Window: %s", SDL_GetError());
    }
}

SDL_AppResult SOT_InitializePipeline(AppState *as) {

    // Initialize base path used to load assets
    InitializeAssetsLoader();

    // Create the shaders
	SDL_GPUShader* vertexShader = LoadShader(as->gpuDevice, "shaderTexture_MVP.vert", 0, 2, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragmentShader = LoadShader(as->gpuDevice, "shaderTexture_MVP.frag", 2, 0, 0, 0);
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
				.format = SDL_GetGPUSwapchainTextureFormat(as->gpuDevice, as->pWindow)
			}},
		},
	};

    as->renderingPipeline = SDL_CreateGPUGraphicsPipeline(as->gpuDevice, &pipelineCreateInfo);
	if (as->renderingPipeline == NULL)
	{
		SDL_Log("Failed to create fill pipeline!");
		return SDL_APP_FAILURE;
	}

    // Clean up shader resources
	SDL_ReleaseGPUShader(as->gpuDevice, vertexShader);
	SDL_ReleaseGPUShader(as->gpuDevice, fragmentShader);

    as->textureSampler_1 = SDL_CreateGPUSampler(as->gpuDevice, &(SDL_GPUSamplerCreateInfo) {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	});

    as->textureSampler_2 = SDL_CreateGPUSampler(as->gpuDevice, &(SDL_GPUSamplerCreateInfo) {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	});
    
}
