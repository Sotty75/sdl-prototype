#include "sot_texture.h"

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

SOT_Texture *CreateTexture(char *name, AppState* appState) {
    SDL_Surface *spritesheetSurface = NULL;
    GetSurfaceFromImage(&spritesheetSurface, name);
    SDL_Texture *txt = SDL_CreateTextureFromSurface(appState->renderer, spritesheetSurface);
    if (txt == NULL) {
            SDL_Log("Couldn't create texture from surface: %s", SDL_GetError());
            return NULL;
    }
    SDL_SetTextureScaleMode(txt, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(spritesheetSurface);

    SOT_Texture *sotTexture = malloc(sizeof(SOT_Texture));
    sotTexture->name = name;
    sotTexture->texture = txt;
    sotTexture->next = NULL;

    return sotTexture;
}


SDL_Texture *GetTexture(char *name, AppState* appState) {

    SOT_Texture *sotTexture = appState->texturesPool;

    if (sotTexture == NULL) {
        sotTexture = CreateTexture(name, appState);
        appState->texturesPool = sotTexture;
        return sotTexture->texture;
    }

    // look for a texture with the same name
    if (appState->texturesPool != NULL) {
        for (int i = 0; sotTexture != NULL; i++) {
            if (strcmp(sotTexture->name, name) == 0) {
                return sotTexture->texture;
            }

            if (sotTexture->next == NULL) {
                sotTexture->next = CreateTexture(name, appState);
                return sotTexture->next->texture;
            }
            else {
                sotTexture = sotTexture->next; 
            }
        }
        
    }
}

void DestroyTexturePool(AppState* appState) {
    SOT_Texture *sotTexture = appState->texturesPool;
    
    if (appState->texturesPool != NULL) 
    {
        for (int i = 0; sotTexture != NULL; i++) 
        {
            SOT_Texture *nextTexture = sotTexture->next;
            SDL_DestroyTexture(sotTexture->texture);
            free(sotTexture);
            sotTexture = nextTexture;
        }
    }
}
