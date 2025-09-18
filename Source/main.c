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
#include <SDL3_image/SDL_image.h>
#include "sprites.h"

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint64 last_step;
} AppState;

SDL_Surface *monkeySpriteSheet = NULL;
Sprite *monkey = NULL;

/*
    Takes a pointer to a 'SDL_Surface *' as an input
    and an image and creates a surface linked by the surface pointer
    to be reused to create for example a texture.
*/
SDL_AppResult getSurfaceFromImage(SDL_Surface **surface, char *assetName)
{
    char *spritesheetPath = NULL;

    //... load the spritesheet inside of the texture
    SDL_asprintf(&spritesheetPath, "%sassets\\%s", SDL_GetBasePath(), assetName);  /* allocate a string of the full file path */
    *surface = IMG_Load(spritesheetPath);

    if (!(*surface)) {
        SDL_Log("Couldn't load spritesheet: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    free(spritesheetPath);

    return SDL_APP_CONTINUE;
}


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) {
        return SDL_APP_FAILURE;
    }

    *appstate = as;


    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    if (!SDL_CreateWindowAndRenderer("Test Creazione Finestra", 1024, 768, 0, &as->window, &as->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderLogicalPresentation(as->renderer, 320, 200, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    //... load the spritesheet inside of the texture
    getSurfaceFromImage(&monkeySpriteSheet, "monkey-sheet.png");

    monkey = CreateSprite("Monkey", monkeySpriteSheet, 0, 8, 16, 16, 125);
    
    as->last_step = SDL_GetTicks();

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *)appstate;
    const Uint64 now = SDL_GetTicks();

    // run game logic if we're at or past the time to run it.
    // if we're _really_ behind the time to run it, run it
    // several times.
    // while ((now - as->last_step) >= STEP_RATE_IN_MILLISECONDS) {
    //     snake_step(ctx);
    //     as->last_step += STEP_RATE_IN_MILLISECONDS;
    // }


    /* clear the window to the draw color. */
    SDL_SetRenderDrawColor(as->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(as->renderer);

    // Start a new rendering pass
    UpdateSprite(monkey, as->renderer, 0, 0);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(as->renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    
}
