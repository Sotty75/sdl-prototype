/* clear.c ... */

/*
 * This example code creates an SDL window and renderer, and then clears the
 * window to a different color every frame, so you'll effectively get a window
 * that's smoothly fading between colors.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */



#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include "box2d.h"
#include "sot_engine.h"
#include "common.h"
#include "cglm.h"
#include "test_main.h"

AppState *as = NULL;
static Scene *currentScene = NULL;    
SDL_Gamepad *gamepad = NULL;
sot_world *world = NULL;
SOT_Camera *camera = NULL;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) { return SDL_APP_FAILURE; }
    
    // Initialize the graphics system
    SOT_InitializeWindow(as);
    SOT_InitializePipeline(as);
    


    //... load the texture file
    SDL_Surface *wallSurface = NULL;
    SDL_Surface *snowSurface = NULL;
    GetSurfaceFromImage(&wallSurface, "textures\\wall.png");
    GetSurfaceFromImage(&snowSurface, "textures\\snow.png");
    
    // Createt the test world
    world = TEST_CreateWorld(9);

 	as->texture_1 = SDL_CreateGPUTexture(as->gpuDevice, &(SDL_GPUTextureCreateInfo) {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = wallSurface->w,
		.height = wallSurface->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
	});
    
 	as->texture_2 = SDL_CreateGPUTexture(as->gpuDevice, &(SDL_GPUTextureCreateInfo) {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = snowSurface->w,
		.height = snowSurface->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
	});

    // ------------------------------ Vertext Data Buffer - START ------------------------------------------//
    SDL_GPUBufferCreateInfo vertexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = world->vertexBufferSize,
        .props = 0
    };

    as->vertexBuffer = SDL_CreateGPUBuffer(as->gpuDevice, &vertexBufferInfo);
    if (as->vertexBuffer == NULL)
	{
		SDL_Log("Failed to create vertex buffer!");
		return SDL_APP_FAILURE;
	}

    SDL_GPUTransferBufferCreateInfo transferBufferInfo = 
    {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size =  world->vertexBufferSize,
        .props = 0
    };
    SDL_GPUTransferBuffer *transferBuffer = SDL_CreateGPUTransferBuffer(as->gpuDevice, &transferBufferInfo);
    if (transferBuffer == NULL)
	{
		SDL_Log("Failed to create transfer buffer!");
		return -SDL_APP_FAILURE;
	}

    vertex* transferData = SDL_MapGPUTransferBuffer(as->gpuDevice,transferBuffer,	false);
    for (int i = 0; i < world->size; ++i) {
        SDL_memcpy(transferData,  world->quadsArray[i].verts, sizeof(world->quadsArray[i].verts));
        transferData = transferData + 4;
    }
      

    SDL_UnmapGPUTransferBuffer(as->gpuDevice, transferBuffer);
    
    // ------------------------------ Index Data Buffer - START ------------------------------------------//
    
    SDL_GPUBufferCreateInfo indexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = world->indexBufferSize,
        .props = 0
    };

    as->indexBuffer = SDL_CreateGPUBuffer(as->gpuDevice, &indexBufferInfo);
    if (as->indexBuffer == NULL)
	{
		SDL_Log("Failed to create index buffer!");
		return SDL_APP_FAILURE;
	}


    SDL_GPUTransferBuffer *indexTransferBuffer = SDL_CreateGPUTransferBuffer
    (as->gpuDevice, 
        &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = world->indexBufferSize,
        .props = 0
    });
    
    if (indexTransferBuffer == NULL)
	{
		SDL_Log("Failed to create transfer buffer!");
		return SDL_APP_FAILURE;
	}

    uint16_t* transferIndexData = SDL_MapGPUTransferBuffer(as->gpuDevice, indexTransferBuffer, false);
    for (int i = 0; i < world->size; ++i) {
        SDL_memcpy(transferIndexData ,  world->quadsArray[i].indexes, sizeof(world->quadsArray[i].indexes));
        transferIndexData = transferIndexData + 6;
    }
    
    SDL_UnmapGPUTransferBuffer(as->gpuDevice, indexTransferBuffer);

    // ------------------------------ Texture Data Buffer ------------------------------------------//
	
    SDL_GPUTransferBufferCreateInfo textureTransferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (wallSurface->w * wallSurface->h * 4) + (snowSurface->w * snowSurface->h * 4)
	};
    
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(as->gpuDevice, &textureTransferBufferInfo);
    if (textureTransferBuffer == NULL)
	{
		SDL_Log("Failed to create texture transfer buffer!");
		return -SDL_APP_FAILURE;
	}
    
	Uint8* textureData = SDL_MapGPUTransferBuffer(as->gpuDevice, textureTransferBuffer,	false);
	SDL_memcpy(textureData, wallSurface->pixels, wallSurface->w * wallSurface->h * 4);
    Uint8* textureData_2 = textureData + (wallSurface->w * wallSurface->h * 4);
    SDL_memcpy(textureData_2, snowSurface->pixels, snowSurface->w * snowSurface->h * 4);
	SDL_UnmapGPUTransferBuffer(as->gpuDevice, textureTransferBuffer);

    //---------------------------------------- Upload Vertex/Index/Textures Buffers to the GPU ------------------------------//

	// Upload the transfer data to the vertex buffer
	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(as->gpuDevice);
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
			.buffer = as->vertexBuffer,
			.offset = 0,
			.size =  world->vertexBufferSize
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
			.buffer = as->indexBuffer,
			.offset = 0,
			.size =  world->indexBufferSize
		},
		false
	);

    SDL_UploadToGPUTexture(
		copyPass,
		&(SDL_GPUTextureTransferInfo) {
			.transfer_buffer = textureTransferBuffer,
			.offset = 0, /* Zeros out the rest */
		},
		&(SDL_GPUTextureRegion){
			.texture = as->texture_1,
			.w = wallSurface->w,
			.h = wallSurface->h,
			.d = 1
		},
		false
	);

      SDL_UploadToGPUTexture(
		copyPass,
		&(SDL_GPUTextureTransferInfo) {
			.transfer_buffer = textureTransferBuffer,
			.offset = wallSurface->w * wallSurface->h * 4, /* Zeros out the rest */
		},
		&(SDL_GPUTextureRegion){
			.texture = as->texture_2,
			.w = snowSurface->w,
			.h = snowSurface->h,
			.d = 1
		},
		false
	);



	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
	SDL_ReleaseGPUTransferBuffer(as->gpuDevice, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(as->gpuDevice, indexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(as->gpuDevice, textureTransferBuffer);
    
    /* Create the camera entity */
    SOT_CameraInfo cameraInfo = {
        .center = {0, 0, 0},
        .eye = {0,0,0},
        .up = {0,1,0}
    };
    SOT_ProjectionInfo projectionInfo = {
        .aspect = SCREEN_WIDTH / SCREEN_HEIGHT,
        .far = 100,
        .fov = 45,
        .near = 5,
        .mode = SOT_PERSPECTIVE,
    };
    camera = CreateCameraWitInfo(cameraInfo, projectionInfo);
    
    /** Commented while implementing the SDL_GPU Logic
       
        // SDL_SetRenderLogicalPresentation(as->pRenderer, 320, 256, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
        
        //...gamepad initialization logic
        int gamepadsCount = 0;
        SDL_JoystickID * gamepads =  SDL_GetGamepads(&gamepadsCount);
        if (gamepads != NULL && gamepadsCount > 0) { gamepad = SDL_OpenGamepad(gamepads[0]); }

        // Initialize our main scene
        currentScene = CreateScene(as);
        if (currentScene == NULL) return SDL_APP_FAILURE;
    **/   

    //...store initialized application state so it will be shared in other
    // functions.
    as->pTexturesPool = NULL;
    as->full_screen_enabled = false;
    as->last_step = SDL_GetTicks();
    as->debugInfo.displayColliders = false;
    *appstate = as;
   
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    /*
    AppState *as = (AppState *)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  
    }

    switch (event->type)
    {
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            MoveActor(currentScene->player, event);
            break;
        case SDL_EVENT_KEY_DOWN:
            switch (event->key.scancode) {
                case SDL_SCANCODE_0:
                    as->full_screen_enabled = !as->full_screen_enabled;
                    SDL_SetWindowFullscreen(as->pWindow, as->full_screen_enabled);
                    break;
                case SDL_SCANCODE_1:
                    as->debugInfo.displayColliders = !as->debugInfo.displayColliders;
                    break;
                default:
                    break;
                }
            break;
        default:
            break;
    }
    */
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    
    AppState *as = (AppState *)appstate;
    const Uint64 now = SDL_GetTicks();
    const float deltaTime = (now - as->last_step) / 1000.0f; // Delta time in seconds
    as->last_step = now;

    /*
        SDL_RenderClear(as->pRenderer);
        // Start a new rendering pass
        UpdateScene(as, currentScene, deltaTime);
        RenderScene(as, currentScene);
        SDL_RenderPresent(as->pRenderer);
    */

    float rad_msec = GLM_PI_4f;
    

    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(as->gpuDevice);
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

    SDL_GPUTextureSamplerBinding textureBindings[] = {
        {
            .texture = as->texture_1, 
            .sampler = as->textureSampler_1
        },
        {
            .texture = as->texture_2, 
            .sampler = as->textureSampler_2
        },
    };

	if (swapchainTexture != NULL)
	{
        mat4 projection_view;
        MoveCamera(camera, (vec3) {0,0,-1}, deltaTime, 1);
        glm_mat4_mul(camera->projection, camera->view, projection_view);
        SDL_PushGPUVertexUniformData(cmdbuf, 1, projection_view, sizeof(projection_view));

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			cmdbuf,
			&colorTargetInfo,
			1,
			NULL
		);

        SDL_BindGPUGraphicsPipeline(renderPass, as->renderingPipeline);
		SDL_BindGPUFragmentSamplers(renderPass, 0, textureBindings, 2);

        mat4 model[9];
        for (int i = 0; i < 9; ++i) {
            // sot_quad_rotation(&(world->quadsArray[i]), rad_msec * deltaTime);
            sot_quad_get_transform_RT(model[i],  &(world->quadsArray[i]));
        }
            
	    SDL_PushGPUVertexUniformData(cmdbuf, 0, model, sizeof(model));
        SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding) { .buffer = as->vertexBuffer, .offset = 0}, 1);
        SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding) {.buffer = as->indexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(renderPass, 6, 3, 0, 0, 0);
		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);
    
	

    return SDL_APP_CONTINUE;  
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{

    return;
    /*
        DestroyTexturePool(as);
        SDL_DestroyRenderer(as->pRenderer);
        SDL_DestroyWindow(as->pWindow);
        SDL_QuitSubSystem(SDL_INIT_VIDEO||SDL_INIT_GAMEPAD);
        SDL_CloseGamepad(gamepad);
        DestroyScene(currentScene);
    */
}
