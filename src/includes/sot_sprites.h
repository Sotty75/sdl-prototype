#ifndef SOT_SPRITES_H
#define SOT_SPRITES_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>


/*
   Struct to store an individual frame LP_Frame
   LP_Frame * - Pinter to next frame of the animation
   SDL_Surface * fs - with frame pixel information
*/
typedef struct Frame {
    struct Frame *next;
    SDL_FRect *sprite;
} Frame;


/*
/   Struct to store animation info LP_Animation
/
/       - animation name
/       - frames count
/       - frame->width
/       - frame->height
/       - frames array
*/
typedef struct Sprite {
    char *name;
    bool cycle;
    int framesCount;
    int width;
    int height;
    int stepRateMillis;
    SDL_Texture *atlas;
    Frame *currentFrame;
} Sprite;



// Prototipi delle funzioni che usano la struttura
SDL_AppResult GetSurfaceFromImage(SDL_Surface **surface, char *assetName);
Sprite *CreateSprite(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, int stepRateMillis, bool cycle, SDL_Renderer *renderer);
void DestroySprite(Sprite *animation);


#endif // SPRITES_H