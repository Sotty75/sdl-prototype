#include "sot_sprite.h"

/*
 * Create an animation structure made of frames where each frame is loaded
 * from a spritesheet. the sprites in the spitesheet have to be sorted in order so that we can index the source 
 * sprite efrom the spritesheet by providing the index of the first sprite, the index of the last sprite and the size of each sprite
 */
sot_sprite_t *CreateAnimation(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, 
    int stepRateMillis, bool cycle,  AppState *appstate)
{
    // ..initialize the animation structure
    sot_sprite_t *animation = malloc(sizeof(sot_sprite_t));
    if (animation == NULL) return NULL; 

    animation->name = name;
    animation->framesCount = (endIndex - startIndex) + 1;
    animation->width = width;
    animation->height = height;
    animation->stepRateMillis = stepRateMillis;
    animation->currentFrame = NULL;
    animation->cycle = cycle;

    // Load the spritesheet in a texture atlas
    animation->atlas = GetTexture(appstate, "monkey-sheet-16.png");

    // ...we use a variable to store the previous frame for the current iteration, 
    // so that we can build the chain of frames.
    Frame *previousFrame = NULL;

    int spritesPerRow = animation->atlas->w / width;
    int spritesPerColumn = animation->atlas->h / height;

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
void DestroySprite(sot_sprite_t *animation) {
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






