#include <stdlib.h>
#include "sprites.h"


/*
 * Create an animation structure made of frames where each frame is loaded
 * from a spritesheet. the sprites in the spitesheet have to be sorted in order so that we can index the source 
 * sprite efrom the spritesheet by providing the index of the first sprite, the index of the last sprite and the size of each sprite
 */
Sprite *CreateSprite(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, int stepRateMillis)
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
    animation->texture = NULL;
    animation->postion.x = 0;
    animation->postion.y = 0;
    animation->postion.w = width;
    animation->postion.h = height;

    // ...we use a variable to store the previous frame for the current iteration, 
    // so that we can build the chain of frames.
    Frame *previousFrame = NULL;

    int spritesPerRow = spritesheet->w / width;
    int spritesPerColumn = spritesheet->h / height;

    // ...we iteratie across all the sprites in the spritesheet there are making up the animation
    // and each iteration we recalculate the source rectangle to match the position of the sprite of
    // for the frame with index i
    for (int frameIndex = startIndex; frameIndex <= endIndex; frameIndex++)
    {
        SDL_Surface *currentFrameSurface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
        if (currentFrameSurface == NULL) return NULL;


        int xIndex =  frameIndex % spritesPerRow;
        int yIndex =  frameIndex / spritesPerRow;

        //...new sorce rectangle
        SDL_Rect sourceFrameRect = {
            .x = xIndex * width,
            .y = yIndex * height,
            .w = width,
            .h = height
        };

        
        //...the current sprite is blitted from the spritesheet into the frame surface
        SDL_BlitSurface(spritesheet, &sourceFrameRect, currentFrameSurface, NULL);

        // ...we can now build the Frame structure for the current frame, which basically
        // is made of the frame surface and a link to the next frame.
        Frame *currentFrame = malloc(sizeof(Frame));
        if (currentFrame == NULL) return NULL;
        
        currentFrame->fs = currentFrameSurface;
        if (previousFrame != NULL) {
            previousFrame->next = currentFrame;
        } else {
            animation->currentFrame = currentFrame;     // we set the first frame of the animation
        }
        currentFrame->next = NULL;                      // Initialize next to NULL for the current frame
        previousFrame = currentFrame;

        // ...we also want the last frame of the animation to link the first frame, as we need a circular animation so we can 
        // implement the animation in a while cycle.
        if (frameIndex == endIndex - 1) {
            previousFrame->next = animation->currentFrame;
        }
    }

    return animation;
}


/*
* Updates the target texture with the next animation frame based on the animation speed.
* returns false in case of errors in the execution of the SDL allocs.
*/
bool UpdateSprite(Sprite *animation, SDL_Renderer *renderer, int xPos, int yPos) 
{
    // If the texture is NULL, create it from the first frame.
    if (animation->texture == NULL) {
            animation->texture = SDL_CreateTextureFromSurface(renderer, animation->currentFrame->fs);
            SDL_SetTextureScaleMode(animation->texture, SDL_SCALEMODE_NEAREST);

            if (animation->texture == NULL) {
                return false;
            }
    }

    // Get the current time different from last step
    // if higher than the animation time step
    // update the texture with the new frame surface.
    const Uint64 now = SDL_GetTicks();

    // while ((now - as->last_step) >= STEP_RATE_IN_MILLISECONDS) {
    //     snake_step(ctx);
    //     as->last_step += STEP_RATE_IN_MILLISECONDS;
    // }


    SDL_RenderTexture(renderer, animation->texture, NULL, &(animation->postion));

    return true;
}

/*
*   Free the memory allocated for the animation
*   in the heap.
*/
void DestroySprite(Sprite *animation) {
    Frame *currentFrame = animation->currentFrame; 
    for (int i = 0; i < animation->framesCount; i++) 
    {
        SDL_DestroySurface(currentFrame->fs);
        Frame *nextFrame = currentFrame->next;
        free(currentFrame);

        currentFrame = nextFrame;
    }
    free(animation);
}






