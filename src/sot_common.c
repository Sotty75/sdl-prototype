#include "sot_common.h"

static const char* BasePath = NULL;
void InitializeAssetsLoader()
{
	BasePath = SDL_GetBasePath();
}

SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename) {
	// open the file
	char fullPath[256];
	SDL_snprintf(fullPath, sizeof(fullPath), "%sassets\\animations\\%s", BasePath, animationsFilename);
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

    // access the JSON data
    cJSON *atlas_name = cJSON_GetObjectItemCaseSensitive(json, "atlas_name");
    if (cJSON_IsString(atlas_name) && (atlas_name->valuestring != NULL)) {
		animationInfo->atlasName = (char *)malloc(SDL_strlen(atlas_name->valuestring));
		 SDL_strlcpy(animationInfo->atlasName, atlas_name->valuestring, SDL_strlen(atlas_name->valuestring));
    }

    cJSON *image_path = cJSON_GetObjectItemCaseSensitive(json, "image_path");
	if (cJSON_IsString(image_path) && (image_path->valuestring != NULL)) {
		animationInfo->atlasPath = (char *)malloc(SDL_strlen(image_path->valuestring));
		SDL_strlcpy(animationInfo->atlasPath, image_path->valuestring, SDL_strlen(image_path->valuestring));
    }

	int i = 0;
	cJSON *animation = NULL;
	cJSON *animations = cJSON_GetObjectItemCaseSensitive(json, "animations");

	cJSON_ArrayForEach(animation, animations)
	{
		cJSON *frame_count = cJSON_GetObjectItemCaseSensitive(animation, "frame_count");
		animationInfo->framesInfo[i].framesCount = frame_count->valueint;

		animationInfo->framesInfo[i].frames = (vec4*) malloc(frame_count->valueint * sizeof(vec4));
		int j = 0;
		cJSON *frame = NULL;
		cJSON *frames = cJSON_GetObjectItemCaseSensitive(animation, "frames");
		cJSON_ArrayForEach(frame, frames) 
		{
			// read x value
			cJSON *x_value = cJSON_GetObjectItemCaseSensitive(frame, "x");
			animationInfo->framesInfo[i].frames[j][0] = x_value->valueint;
	
			// read y value
			cJSON *y_value = cJSON_GetObjectItemCaseSensitive(frame, "y");
			animationInfo->framesInfo[i].frames[j][1] = y_value->valueint;

			// read w value
			cJSON *w_value = cJSON_GetObjectItemCaseSensitive(frame, "w");
			animationInfo->framesInfo[i].frames[j][2] = w_value->valueint;

			// read h value
			cJSON *h_value = cJSON_GetObjectItemCaseSensitive(frame, "h");
			animationInfo->framesInfo[i].frames[j][3] = h_value->valueint;

			j++;
		}
		
		++i;
	}

    // delete the JSON object
    cJSON_Delete(json);
	return animationInfo;
}

SDL_Surface* LoadImage(const char* imageFilename, int desiredChannels)
{
	char fullPath[256];
	SDL_Surface *result;
	SDL_PixelFormat format;
	SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets\\%s", BasePath, imageFilename);

	result = SDL_LoadBMP(fullPath);
	if (result == NULL)
	{
		SDL_Log("Failed to load BMP: %s", SDL_GetError());
		return NULL;
	}

	if (desiredChannels == 4)
	{
		format = SDL_PIXELFORMAT_RGBA32;
	}
	else
	{
		SDL_assert(!"Unexpected desiredChannels");
		SDL_DestroySurface(result);
		return NULL;
	}

	if (result->format != format)
	{
		SDL_Surface *next = SDL_ConvertSurface(result, format);
		SDL_DestroySurface(result);
		result = next;
	}

	return result;
}

/// @brief 
/// @param device - a reference to the SDL_GPUDevice object
/// @param shaderFilename - shaders name
/// @param samplerCount - number of texture samplers to be bound to the shader
/// @param uniformBufferCount - number of uniform buffers to be bound to the shader
/// @param storageBufferCount - number of storage buffers to be bound to the shader
/// @param storageTextureCount - number of storage textures to be bound to the shader
/// @return 
SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	Uint32 samplerCount,
	Uint32 uniformBufferCount,
	Uint32 storageBufferCount,
	Uint32 storageTextureCount
) {
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets/Shaders/Compiled/SPIRV/%s.spv", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets/Shaders/Compiled/MSL/%s.msl", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets/Shaders/Compiled/DXIL/%s.dxil", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load shader from disk! %s", fullPath);
		return NULL;
	}

	SDL_GPUShaderCreateInfo shaderInfo = {
		.code = code,
		.code_size = codeSize,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = samplerCount,
		.num_uniform_buffers = uniformBufferCount,
		.num_storage_buffers = storageBufferCount,
		.num_storage_textures = storageTextureCount
	};

	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
) {
	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets/Shaders/Compiled/SPIRV/%s.spv", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets/Shaders/Compiled/MSL/%s.msl", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets/Shaders/Compiled/DXIL/%s.dxil", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load compute shader from disk! %s", fullPath);
		return NULL;
	}

	// Make a copy of the create data, then overwrite the parts we need
	SDL_GPUComputePipelineCreateInfo newCreateInfo = *createInfo;
	newCreateInfo.code = code;
	newCreateInfo.code_size = codeSize;
	newCreateInfo.entrypoint = entrypoint;
	newCreateInfo.format = format;

	SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &newCreateInfo);
	if (pipeline == NULL)
	{
		SDL_Log("Failed to create compute pipeline!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return pipeline;
}