#include "sot_texture.h"

/*
    Takes a pointer to a 'SDL_Surface *' as an input
    and an image and creates a surface linked by the surface pointer
    to be reused to create for example a texture.
*/
SDL_AppResult GetSurfaceFromImage(SDL_Surface **surface, char *folderName, char *assetName)
{
    char *assetPath = NULL;

    //... load the spritesheet inside of the texture
    SDL_asprintf(&assetPath, "%sassets\\%s\\%s", SDL_GetBasePath(), folderName, assetName);  /* allocate a string of the full file path */
    *surface = IMG_Load(assetPath);

    if (!(*surface)) {
        SDL_Log("Couldn't load spritesheet: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_free(assetPath);

    return SDL_APP_CONTINUE;
}

/*
Creates a texture, this is not adding the texture automatically to the textures pool.
The texture pool is managed by the GetTexture function, which checks if the texture is in the pool.
If not, it will be created using this function, added to the pool, and returned to the user.
*/
sot_texture_t *CreateTexture(AppState* appState, char *fileName) {
    SDL_Surface *spritesheetSurface = NULL;
    GetSurfaceFromImage(&spritesheetSurface, "textures", fileName);
    SDL_Texture *txt = SDL_CreateTextureFromSurface(appState->gpu->renderer, spritesheetSurface);
    if (txt == NULL) {
            SDL_Log("Couldn't create texture from surface: %s", SDL_GetError());
            return NULL;
    }
    SDL_SetTextureScaleMode(txt, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(spritesheetSurface);

    sot_texture_t *sotTexture = malloc(sizeof(sot_texture_t));
    sotTexture->name = fileName;
    sotTexture->texture = txt;
    sotTexture->next = NULL;

    return sotTexture;
}


SDL_Texture *GetTexture(AppState* appState, char *name) {

    sot_texture_t *sotTexture = appState->pTexturesPool;

    if (sotTexture == NULL) {
        sotTexture = CreateTexture(appState, name);
        appState->pTexturesPool = sotTexture;
        return sotTexture->texture;
    }

    // look for a texture with the same name
    if (appState->pTexturesPool != NULL) {
        for (int i = 0; sotTexture != NULL; i++) {

            // if the texture is found in the pool,
            // return the texture.
            if (strcmp(sotTexture->name, name) == 0) {
                return sotTexture->texture;
            }

            // if we reeach the end of the pool we create a new texture
            // and add it to the pool. 
            // then it is returned to the user.
            if (sotTexture->next == NULL) {
                sotTexture->next = CreateTexture(appState, name);
                return sotTexture->next->texture;
            }
            else {
                sotTexture = sotTexture->next; 
            }
        }
        
    }
}

void DestroyTexturePool(AppState* appState) {
    sot_texture_t *sotTexture = appState->pTexturesPool;
    
    if (appState->pTexturesPool != NULL) 
    {
        for (int i = 0; sotTexture != NULL; i++) 
        {
            sot_texture_t *nextTexture = sotTexture->next;
            SDL_DestroyTexture(sotTexture->texture);
            free(sotTexture);
            sotTexture = nextTexture;
        }
    }
}
