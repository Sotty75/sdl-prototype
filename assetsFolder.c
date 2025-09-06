#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #define GET_CURRENT_DIR _getcwd
#else
    #include <unistd.h>
    #define GET_CURRENT_DIR getcwd
#endif

/**
 * Concatenates two strings and returns a new, dynamically allocated string.
 * The caller is responsible for freeing the returned memory with free().
 * Returns NULL on failure.
 */
char *appendString(const char *str1, const char *str2)
{
    // 1. Calculate the exact memory needed, including the null terminator
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char *result = malloc(len1 + len2 + 1); // +1 for '\0'

    if (!result) {
        perror("Failed to allocate memory in appendString");
        return NULL;
    }

    // 2. Efficiently copy the strings into the new buffer
    memcpy(result, str1, len1);
    memcpy(result + len1, str2, len2 + 1); // Copy str2 and its null terminator

    return result;
}

/**
 * Gets the project's assets folder path.
 * This path is calculated only once and stored for subsequent calls.
 * The returned string should NOT be freed by the caller.
 */
char *getAssetsFolder()
{
    static char *assetsFolder = NULL; // Initialize to NULL

    // Only run this logic the first time the function is called
    if (assetsFolder == NULL)
    {
        char current_dir[1024]; // A temporary buffer on the stack

        // Get the current working directory
        if (!GET_CURRENT_DIR(current_dir, sizeof(current_dir))) {
            perror("Failed to get current directory");
            return NULL;
        }

        // Use snprintf to safely create the final path.
        // First, calculate the required size.
        size_t required_size = snprintf(NULL, 0, "%s\\Assets\\", current_dir) + 1;
        
        assetsFolder = malloc(required_size);
        if (!assetsFolder) {
            perror("Failed to allocate memory for assetsFolder");
            return NULL;
        }
        
        // Now, write the formatted string into the allocated buffer.
        snprintf(assetsFolder, required_size, "%s\\Assets\\", current_dir);
    }

    return assetsFolder;
}