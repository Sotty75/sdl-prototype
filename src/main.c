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
#include <box2d/box2d.h>
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
    SOT_InitializePipelineWithInfo(as, &(SOT_GPU_PipelineInfo) {
        .pipeline_ID = SOT_RP_TILEMAP,
        .vertexShaderName = "shaderTexture_MVP.vert",
        .fragmentShaderName = "shaderTexture_MVP.frag"
    });

    //... load the texture file
    SDL_Surface *wallSurface = NULL;
    SDL_Surface *snowSurface = NULL;
    GetSurfaceFromImage(&wallSurface, "textures\\wall.png");
    GetSurfaceFromImage(&snowSurface, "textures\\snow.png");
    
    // Createt the test world
    world = TEST_CreateWorld(9);
 	
    // ------------------------------ Vertext Data Buffer - START ------------------------------------------//

    SOT_GPU_Data gpuData = {0};
    gpuData.vertexDataSize = QUAD_VERTS * sizeof(vertex);
    gpuData.vertexData = (vertex *) malloc(gpuData.vertexDataSize);
    memcpy(gpuData.vertexData, world->quad->verts, gpuData.vertexDataSize);

    gpuData.indexDataSize = QUAD_INDEXES * sizeof(uint16_t);
    gpuData.indexData = (uint16_t *) malloc(gpuData.indexDataSize);
    memcpy(gpuData.indexData, world->quad->indexes, gpuData.indexDataSize);

    gpuData.surfaces[0] = wallSurface;
    gpuData.surfaces[1] = snowSurface;
    gpuData.surfaceCount = 2;
    
    SOT_UploadBufferData(as->gpu, &gpuData);
    
    
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
        // Start a new rendering pass
        UpdateScene(as, currentScene, deltaTime);
        RenderScene(as, currentScene);

    */

    float rad_msec = GLM_PI_4f;
    for (int i = 0; i < world->count; ++i) {
        // sot_quad_rotation(&(world->quadsArray[i]), rad_msec * deltaTime);
        sot_quad_get_transform_RT(world->transforms[i],  &(world->quads[i]));
    }

    mat4 projection_view;
    MoveCamera(camera, (vec3) {0,0,-1}, deltaTime, 1);
    glm_mat4_mul(camera->projection, camera->view, projection_view);

    SOT_RenderScene(as, world->transforms, world->count, projection_view);

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
