#include "sot_animation.h"



SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename) 
{
	char *fullPath;
	SDL_asprintf(&fullPath, "%s\\%s", Paths.Animations, animationsFilename);

    //...open the file and read the file contents into a string
    FILE *fp = fopen(fullPath, "r");
    if (fp == NULL) {
        SDL_Log("Error: Unable to open the animations file %s.\n", fullPath);
        SDL_free(fullPath);
        return NULL;
    }
    SDL_free(fullPath);
	
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    char *content = (char *)SDL_calloc(fileSize + 1, 1);
    int len = fread(content, 1, fileSize, fp);
    fclose(fp);

	// parse the JSON data
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        SDL_free(content);
        return NULL;
    }

	SOT_AnimationInfo *animationInfo = (SOT_AnimationInfo *) SDL_calloc(1, sizeof(SOT_AnimationInfo));

	// Allocate sequences array dynamically
	cJSON *animations = cJSON_GetObjectItemCaseSensitive(json, "animations");
	int sequenceCount = cJSON_GetArraySize(animations);
	animationInfo->sequences = (SOT_AnimationSequence *) SDL_calloc(sequenceCount, sizeof(SOT_AnimationSequence));
	if (animationInfo->sequences == NULL) {
		SDL_Log("Error: Failed to allocate sequences array");
		SDL_free(animationInfo);
		cJSON_Delete(json);
		SDL_free(content);
		return NULL;
	}

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

	cJSON *step_ms = cJSON_GetObjectItemCaseSensitive(json, "step_ms");
	if (cJSON_IsNumber(step_ms)) {
		animationInfo->step_ms = (uint16_t)step_ms->valueint;
	} else {
		animationInfo->step_ms = 75;
	}

	animationInfo->count = 0;
	cJSON *animation = NULL;

	cJSON_ArrayForEach(animation, animations)
	{
        int i = animationInfo->count;

		if (animation->string != NULL)
		{
			int sl = SDL_strlen(animation->string) + 1;
			animationInfo->sequences[i].name = (char *)SDL_malloc(sl);
			SDL_strlcpy(animationInfo->sequences[i].name, animation->string, sl);
		}

		cJSON *frame_count = cJSON_GetObjectItemCaseSensitive(animation, "frame_count");
		if (frame_count == NULL || !cJSON_IsNumber(frame_count)) continue;
		animationInfo->sequences[i].count = frame_count->valueint;
		animationInfo->sequences[i].frames = (vec4*) SDL_malloc(frame_count->valueint * sizeof(vec4));


		int j = 0;
		cJSON *frame = NULL;
		cJSON *frames = cJSON_GetObjectItemCaseSensitive(animation, "frames");
		cJSON_ArrayForEach(frame, frames) 
		{
			cJSON *x_value = cJSON_GetObjectItemCaseSensitive(frame, "x");
			cJSON *y_value = cJSON_GetObjectItemCaseSensitive(frame, "y");
			cJSON *w_value = cJSON_GetObjectItemCaseSensitive(frame, "w");
			cJSON *h_value = cJSON_GetObjectItemCaseSensitive(frame, "h");
			if (!x_value || !y_value || !w_value || !h_value) continue;

			animationInfo->sequences[i].frames[j][0] = x_value->valueint;
			animationInfo->sequences[i].frames[j][1] = y_value->valueint;
			animationInfo->sequences[i].frames[j][2] = w_value->valueint;
			animationInfo->sequences[i].frames[j][3] = h_value->valueint;

			j++;
		} 

        animationInfo->count++;
	}

    cJSON_Delete(json);
    SDL_free(content);
    
    return animationInfo;
}


