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
#include "sot_common.h"
#include "cglm.h"

//---- Globals Definition -----//
SOT_Paths Paths = {0};
SDL_Gamepad *gamepad = NULL;

//---- Locals Definition -----///
static AppState *as = NULL;
static SOT_Scene *currentScene = NULL; 

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) { return SDL_APP_FAILURE; }
    
    // ----------------------------- Initialize the graphics system -----------------------------------//
    SOT_GPU_InitRenderer(as, SOT_RPF_TILEMAP | SOT_RPF_DEBUG);
    // SOT_GPU_InitRenderer(as, SOT_RPF_TEST);

    // ------------------------------ Initialize the Gamepad ------------------------------------------//
    int gamepadsCount = 0;
    SDL_JoystickID * gamepads =  SDL_GetGamepads(&gamepadsCount);
    if (gamepads != NULL && gamepadsCount > 0) 
         gamepad = SDL_OpenGamepad(gamepads[0]);

    // Initialize our main scene
    currentScene = SOT_InitializeScene(as);
    if (currentScene == NULL) 
        return SDL_APP_FAILURE;

    // World Initialization
    if (as->gpu->pipelineFlags & SOT_RPF_TEST)
        SOT_GPU_InitializeTestData(as->gpu);
    if (as->gpu->pipelineFlags & SOT_RPF_TILEMAP)
        SOT_GPU_InitializeTilemap(currentScene->tilemap, as->gpu);
    
    // SDL_SetRenderLogicalPresentation(as->pRenderer, 320, 256, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
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
    
    AppState *as = (AppState *)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  
    }

    switch (event->type)
    {
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            // MoveActor(currentScene->player, event);
            break;
        case SDL_EVENT_KEY_DOWN:
            switch (event->key.scancode) {
                case SDL_SCANCODE_0:
                    as->full_screen_enabled = !as->full_screen_enabled;
            // SDL_SetWindowFullscreen(as->pWindow, as->full_screen_enabled);
                    break;
                case SDL_SCANCODE_1:
                    as->debugInfo.displayColliders = !as->debugInfo.displayColliders;
                    break;
                case SDL_SCANCODE_ESCAPE:
                    return SDL_APP_SUCCESS;
                default:
                    break;
                }
            break;
        default:
            break;
    }
    
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    
    AppState *as = (AppState *)appstate;
    const Uint64 now = SDL_GetTicks();
    const float deltaTime = (now - as->last_step) / 1000.0f; // Delta time in seconds
    as->last_step = now;

    UpdateScene(as, currentScene, deltaTime);
    SOT_GPU_Render(as->gpu, currentScene);
    
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
