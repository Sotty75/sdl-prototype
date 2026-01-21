#include "sot_animation.h"

/*
 * Create an animation structure made of frames where each frame is loaded
 * from a spritesheet. the sprites in the spitesheet have to be sorted in order so that we can index the source 
 * sprite efrom the spritesheet by providing the index of the first sprite, the index of the last sprite and the size of each sprite
 */
SOT_Animation *CreateAnimation(char *name, SDL_Surface *spritesheet, int startIndex, int endIndex, int width, int height, 
    int stepRateMillis, bool cycle,  AppState *appstate)
{
    /* // ..initialize the animation structure
    SOT_Animation *animation = malloc(sizeof(SOT_Animation));
    if (animation == NULL) return NULL; 

    animation->name = name;
    animation->framesCount = (endIndex - startIndex) + 1;
    animation->info->frameSize[0] = width;
    animation->info->frameSize[1] = height;
    animation->stepRateMillis = stepRateMillis;
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
    } */

    return NULL; //animation;
}


SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename) {
	// open the file
	char fullPath[256];
	SDL_snprintf(fullPath, sizeof(fullPath), "%s\\%s", Paths.Animations, animationsFilename);
    FILE *fp = fopen(fullPath, "r");
    if (fp == NULL) {
        SDL_Log("Error: Unable to open the animations file %s.\n", fullPath);
        return NULL;
    }

	// read the file contents into a string
    char buffer[4096];
    int len = fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

	// parse the JSON data
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return NULL;
    }

	SOT_AnimationInfo *animationInfo = (SOT_AnimationInfo *) malloc(sizeof(SOT_AnimationInfo));

    // Access the JSON data
    cJSON *atlas_name = cJSON_GetObjectItemCaseSensitive(json, "atlas_name");
    if (cJSON_IsString(atlas_name) && (atlas_name->valuestring != NULL)) {
		int sl = SDL_strlen(atlas_name->valuestring) + 1;
		animationInfo->atlasName = (char *)SDL_malloc(sl);
		 SDL_strlcpy(animationInfo->atlasName, atlas_name->valuestring, sl);
    }

    cJSON *image_path = cJSON_GetObjectItemCaseSensitive(json, "image_path");
	if (cJSON_IsString(image_path) && (image_path->valuestring != NULL)) {
		int sl = SDL_strlen(image_path->valuestring) + 1;
		animationInfo->atlasPath = (char *)SDL_malloc(sl);
		SDL_strlcpy(animationInfo->atlasPath, image_path->valuestring, sl);
    }

    cJSON *collider = cJSON_GetObjectItemCaseSensitive(json, "collider");
	if (cJSON_IsString(collider) && (collider->valuestring != NULL)) {
		int sl = SDL_strlen(collider->valuestring) + 1;
		animationInfo->collider = (char *)SDL_malloc(sl);
		SDL_strlcpy(animationInfo->collider, collider->valuestring, sl);
    }

	int i = 0;
	cJSON *animation = NULL;
	cJSON *animations = cJSON_GetObjectItemCaseSensitive(json, "animations");

	cJSON_ArrayForEach(animation, animations)
	{
		if (animation->string != NULL)
		{
			int sl = SDL_strlen(animation->string) + 1;
			animationInfo->data[i].name = (char *)SDL_malloc(sl);
			SDL_strlcpy(animationInfo->data[i].name, animation->string, sl);
		}

		cJSON *frame_count = cJSON_GetObjectItemCaseSensitive(animation, "frame_count");
		animationInfo->data[i].count = frame_count->valueint;

		animationInfo->data[i].frames = (vec4*) SDL_malloc(frame_count->valueint * sizeof(vec4));
		int j = 0;
		cJSON *frame = NULL;
		cJSON *frames = cJSON_GetObjectItemCaseSensitive(animation, "frames");
		cJSON_ArrayForEach(frame, frames) 
		{
			// read x value
			cJSON *x_value = cJSON_GetObjectItemCaseSensitive(frame, "x");
			animationInfo->data[i].frames[j][0] = x_value->valueint;
	
			// read y value
			cJSON *y_value = cJSON_GetObjectItemCaseSensitive(frame, "y");
			animationInfo->data[i].frames[j][1] = y_value->valueint;

			// read w value
			cJSON *w_value = cJSON_GetObjectItemCaseSensitive(frame, "w");
			animationInfo->data[i].frames[j][2] = w_value->valueint;

			// read h value
			cJSON *h_value = cJSON_GetObjectItemCaseSensitive(frame, "h");
			animationInfo->data[i].frames[j][3] = h_value->valueint;

			j++;
		} 

        i++;
	}

    animationInfo->count = i;

    // delete the JSON object
    cJSON_Delete(json);
	return animationInfo;
}


/*
*   Free the memory allocated for the animation
*   in the heap.
*/
void DestroyAnimation(SOT_Animation *animation) {
    /* Frame *currentFrame = animation->currentFrame; 
    for (int i = 0; i < animation->framesCount; i++) 
    {
        // free the sprite rect memory and the current sprite memory
        free(currentFrame->sprite);
        Frame *nextFrame = currentFrame->next;
        free(currentFrame);

        currentFrame = nextFrame;
    }
    free(animation); */
}
