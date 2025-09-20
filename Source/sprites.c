#include <stdlib.h>
#include "sprites.h"

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
    animation->postion.x = 0;
    animation->postion.y = 0;
    animation->postion.w = width;
    animation->postion.h = height;
    animation->cycle = cycle;
    animation->last_step = SDL_GetTicks();
    

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

        //...create the texture for the frame from the surface
        SDL_Texture *currentTexture = SDL_CreateTextureFromSurface(renderer, currentFrameSurface);
        SDL_SetTextureScaleMode(currentTexture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(currentFrameSurface);
        
        if (currentTexture == NULL) {
            SDL_Log("Couldn't create texture from surface: %s", SDL_GetError());
            return NULL;
        }


        // ...we can now build the Frame structure for the current frame, which basically
        // is made of the frame surface and a link to the next frame.
        Frame *currentFrame = malloc(sizeof(Frame));
        currentFrame->texture = currentTexture;
        if (currentFrame == NULL) return NULL;
        
        if (previousFrame != NULL) {
            previousFrame->next = currentFrame;
        } else {
            animation->currentFrame = currentFrame;     // we set the first frame of the animation
        }
        
        // ...we also want the last frame of the animation to link the first frame, as we need a circular animation so we can 
        // implement the animation in a while cycle.
        if (frameIndex == endIndex) 
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
* Updates the target texture with the next animation frame based on the animation speed.
* returns false in case of errors in the execution of the SDL allocs.
*/
bool UpdateSprite(Sprite *animation, int xPos, int yPos, SDL_Renderer *renderer) 
{
    // Get the current time different from last step
    // if higher than the animation time step
    // update the texture with the new frame surface.
    const Uint64 now = SDL_GetTicks();

    // This while will executed once as soon as now reached
    // the value of the stepRateMillis, it will exit at the first execution
    // as we immediately reset last_step to now
    while ((now - animation->last_step) >= animation->stepRateMillis) 
    {
        // navigate to the next frame in the linked list of frames
        animation->currentFrame = animation->currentFrame->next;
        animation->last_step = now;            
    }

    SDL_RenderTexture(renderer, animation->currentFrame->texture, NULL, &(animation->postion));

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
        // free the texture memory
        SDL_DestroyTexture(currentFrame->texture);

        // free the frame memory
        Frame *nextFrame = currentFrame->next;
        free(currentFrame);

        currentFrame = nextFrame;
    }
    free(animation);
}






