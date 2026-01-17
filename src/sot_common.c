#include "sot_common.h"

void InitializePaths() 
{

}

void InitializeAssetsLoader()
{
	Paths.Base = (char *)SDL_GetBasePath();

	char shadersPath[256];
	char texturesPath[256];
	char tilemapsPath[256];
	char animationsPath[256];

	SDL_snprintf(shadersPath, sizeof(shadersPath), "%sAssets/Shaders/", Paths.Base);
	Paths.Shaders = (char *) SDL_malloc(SDL_strlen(shadersPath) + 1);
	SDL_strlcpy(Paths.Shaders, shadersPath, SDL_strlen(shadersPath) + 1);

	SDL_snprintf(texturesPath, sizeof(texturesPath), "%sAssets/Textures/", Paths.Base);
	Paths.Textures = (char *) SDL_malloc(SDL_strlen(texturesPath) + 1);
	SDL_strlcpy(Paths.Textures, texturesPath, SDL_strlen(texturesPath) + 1);

	SDL_snprintf(tilemapsPath, sizeof(tilemapsPath), "%sAssets/Maps/", Paths.Base);
	Paths.TiledMaps = (char *) SDL_malloc(SDL_strlen(tilemapsPath) + 1);
	SDL_strlcpy(Paths.TiledMaps, tilemapsPath, SDL_strlen(tilemapsPath) + 1);

	SDL_snprintf(animationsPath, sizeof(animationsPath), "%sAssets/Animations/", Paths.Base);
	Paths.Animations = (char *) SDL_malloc(SDL_strlen(animationsPath) + 1);
	SDL_strlcpy(Paths.Animations, animationsPath, SDL_strlen(animationsPath) + 1);
}

SDL_Surface* LoadImage(const char* imageFilename, int desiredChannels)
{
	char fullPath[256];
	SDL_Surface *result;
	SDL_PixelFormat format;
	SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets\\%s", Paths.Base, imageFilename);

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
		SDL_snprintf(fullPath, sizeof(fullPath), "%s\\Compiled\\SPIRV\\%s.spv", Paths.Shaders, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s\\Compiled\\MSL\\%s.msl", Paths.Shaders, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s\\Compiled\\DXIL\\%s.dxil", Paths.Shaders, shaderFilename);
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
		SDL_snprintf(fullPath, sizeof(fullPath), "%sAssets\\%s.spv", Paths.Shaders, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s\\%s.msl", Paths.Shaders, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s\\%s.dxil", Paths.Shaders, shaderFilename);
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