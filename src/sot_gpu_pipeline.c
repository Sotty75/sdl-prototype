
#include "sot_engine.h"


SDL_AppResult SOT_InitializePipeline(AppState *as) {

    // Initialize base path used to load assets
    InitializeAssetsLoader();

    // Create the shaders
	SDL_GPUShader* vertexShader = LoadShader(as->gpuDevice, "shaderTexture.vert", 0, 1, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragmentShader = LoadShader(as->gpuDevice, "shaderTexture.frag", 2, 0, 0, 0);
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
    
}
