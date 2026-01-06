#ifndef SOT_SPRITES_H
#define SOT_SPRITES_H

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "sot_texture.h"
#include "sot_gpu_pipeline.h"

/*
/   Struct to store animation info LP_Animation
/
/       - animation name
/       - frames count
/       - frame->width
/       - frame->height
/       - frames array
*/
typedef struct SOT_Animation {
    int id;
    char *name;
    char *atlasName;
    int stepRateMillis;
    int framesCount;
    bool cycle;
    SOT_GPU_SpriteInfo *info;  //-->collection of frames for the current Animation
} SOT_Animation;



// Prototipi delle funzioni che usano la struttura
SOT_Animation *CreateAnimation(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, int stepRateMillis, bool cycle, AppState* appState);
void DestroyAnimation(SOT_Animation *animation);


#endif // SPRITES_H