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
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include "box2d.h"
#include "sot_engine.h"



AppState *as = NULL;
static Scene *currentScene = NULL;    
SDL_Gamepad *gamepad = NULL;




/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) { return SDL_APP_FAILURE; }
    
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Test Creazione Finestra", 1024, 768, 0, &as->pWindow, &as->pRenderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    //...gamepad initialization logic
    int gamepadsCount = 0;
    SDL_JoystickID * gamepads =  SDL_GetGamepads(&gamepadsCount);
    if (gamepads != NULL && gamepadsCount > 0) { gamepad = SDL_OpenGamepad(gamepads[0]); }


    SDL_SetRenderLogicalPresentation(as->pRenderer, 320, 256, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    
    // Initialize our main scene
    currentScene = CreateScene(as);
    if (currentScene == NULL) return SDL_APP_FAILURE;


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
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
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

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *)appstate;

    const Uint64 now = SDL_GetTicks();
    const float deltaTime = (now - as->last_step) / 1000.0f; // Delta time in seconds
    as->last_step = now;

    /* clear the window to the draw color. */
    SDL_SetRenderDrawColor(as->pRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(as->pRenderer);

    // Start a new rendering pass
    UpdateScene(as, currentScene, deltaTime);
    RenderScene(as, currentScene);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(as->pRenderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    DestroyTexturePool(as);
    SDL_DestroyRenderer(as->pRenderer);
    SDL_DestroyWindow(as->pWindow);
    SDL_QuitSubSystem(SDL_INIT_VIDEO||SDL_INIT_GAMEPAD);
    SDL_CloseGamepad(gamepad);
    DestroyScene(currentScene);
    
}
