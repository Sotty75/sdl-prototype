#include "sot_texture.h"

/*
    Takes a pointer to a 'SDL_Surface *' as an input
    and an image and creates a surface linked by the surface pointer
    to be reused to create for example a texture.
*/
SDL_AppResult GetSurfaceFromImage(SDL_Surface **surface, char *assetName)
{
    char *assetPath = NULL;

    //... load the spritesheet inside of the texture
    SDL_asprintf(&assetPath, "%s\\%s", Paths.Textures, assetName);  /* allocate a string of the full file path */
    *surface = IMG_Load(assetPath);
    SDL_free(assetPath);

    if (!(*surface)) {
        SDL_Log("Couldn't load spritesheet: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void DestroyTexturePool(AppState* appState) {
    sot_texture_t *sotTexture = appState->pTexturesPool;
    
    if (appState->pTexturesPool != NULL) 
    {
        for (int i = 0; sotTexture != NULL; i++) 
        {
            sot_texture_t *nextTexture = sotTexture->next;
            SDL_DestroyTexture(sotTexture->texture);
            SDL_free(sotTexture);
            sotTexture = nextTexture;
        }
    }
}
