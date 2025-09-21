#include <stdlib.h>
#include "sot_sprites.h"


/*
    Takes a pointer to a 'SDL_Surface *' as an input
    and an image and creates a surface linked by the surface pointer
    to be reused to create for example a texture.
*/
SDL_AppResult GetSurfaceFromImage(SDL_Surface **surface, char *assetName)
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

/*
 * Create an animation structure made of frames where each frame is loaded
 * from a spritesheet. the sprites in the spitesheet have to be sorted in order so that we can index the source 
 * sprite efrom the spritesheet by providing the index of the first sprite, the index of the last sprite and the size of each sprite
 */
Sprite *CreateSprite(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, 
    int stepRateMillis, bool cycle,  SDL_Renderer *renderer)
{
    // ..initialize the animation structure
    Sprite *animation = malloc(sizeof(Sprite));
    if (animation == NULL) return NULL; 

    animation->name = name;
    animation->framesCount = (endIndex - startIndex) + 1;
    animation->width = width;
    animation->height = height;
    animation->stepRateMillis = stepRateMillis;
    animation->currentFrame = NULL;
    animation->cycle = cycle;

    // Load the spritesheet in a texture atlas
    SDL_Surface *spritesheetSurface = NULL;
    GetSurfaceFromImage(&spritesheetSurface, "monkey-sheet.png");
    animation->atlas = SDL_CreateTextureFromSurface(renderer, spritesheetSurface);
    if (animation->atlas == NULL) {
            SDL_Log("Couldn't create texture from surface: %s", SDL_GetError());
            return NULL;
    }
    SDL_SetTextureScaleMode(animation->atlas, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(spritesheetSurface);

    // ...we use a variable to store the previous frame for the current iteration, 
    // so that we can build the chain of frames.
    Frame *previousFrame = NULL;

    int spritesPerRow = spritesheet->w / width;
    int spritesPerColumn = spritesheet->h / height;

    // ...we iteratie across all the sprites in the spritesheet there are making up the animation
    // and each iteration we recalculate the source rectangle to match the position of the sprite of
    // for the frame with index i
    for (int spriteIndex = startIndex; spriteIndex <= endIndex; spriteIndex++)
    {
        int xIndex =  spriteIndex % spritesPerRow;
        int yIndex =  spriteIndex / spritesPerRow;

        //...new sorce rectangle
        SDL_FRect spriteRect = {
            .x = xIndex * width,
            .y = yIndex * height,
            .w = width,
            .h = height
        };

        // ...we can now build the Frame structure for the current frame, which basically
        // is made of the frame surface and a link to the next frame.
        Frame *currentFrame = malloc(sizeof(Frame));
        currentFrame->sprite = malloc(sizeof(SDL_FRect));
        memcpy(currentFrame->sprite, &spriteRect, sizeof(SDL_FRect));

        if (currentFrame == NULL) return NULL;
        if (previousFrame != NULL) {
            previousFrame->next = currentFrame;
        } else {
            animation->currentFrame = currentFrame;     // we set the first frame of the animation
        }
        
        // ...we also want the last frame of the animation to link the first frame, as we need a circular animation so we can 
        // implement the animation in a while cycle.
        if (spriteIndex == endIndex) 
            currentFrame->next = animation->currentFrame;
        else 
            currentFrame->next = NULL; 

        // ...we finally prepare for the next frame by setting  
        // the currentFrame as previousFrame
        previousFrame = currentFrame;
    }

    return animation;
}

/*
*   Free the memory allocated for the animation
*   in the heap.
*/
void DestroySprite(Sprite *animation) {
    Frame *currentFrame = animation->currentFrame; 
    for (int i = 0; i < animation->framesCount; i++) 
    {
        // free the sprite rect memory and the current sprite memory
        free(currentFrame->sprite);
        Frame *nextFrame = currentFrame->next;
        free(currentFrame);

        currentFrame = nextFrame;
    }
    free(animation);
}






