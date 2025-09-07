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


/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static int texture_width = 0;
static int texture_height = 0;

SDL_Surface *monkeySpriteSheet = NULL;

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
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Test Creazione Finestra", 1024, 768, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderLogicalPresentation(renderer, 320, 200, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    //... load the spritesheet inside of the texture
    getSurfaceFromImage(&monkeySpriteSheet, "monkey-sheet.png");

    // ...define the clipping rectangle for "Frame 1".
    // each frame is 16 bits wide
    SDL_Rect *clip = malloc(sizeof(SDL_Rect));
    clip->x = 0;
    clip->y = 0;
    clip->w = 16;
    clip->h = 16;

    // ...i also need a destination surface for the frame.
    SDL_Surface *frame0 =  SDL_CreateSurface(clip->w, clip->h, SDL_PIXELFORMAT_ABGR8888);
    SDL_BlitSurface(monkeySpriteSheet, clip, frame0, NULL);
    texture_width = clip->w;
    texture_height = clip->h;
    texture = SDL_CreateTextureFromSurface(renderer, frame0);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(frame0);

    if (!texture) {
        SDL_Log("Couldn't create static texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

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
    /* clear the window to the draw color. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(renderer);

    // Start a new rendering pass
    SDL_FRect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = texture_width;
    rect.h = texture_height;
    SDL_RenderTexture(renderer, texture, NULL, &rect);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    
}
