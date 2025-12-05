# Minimum version required for cmake
cmake_minimum_required(VERSION 3.15)

# Project definition: name and language
project(sdl-prototype LANGUAGES C)

# C Version Requried
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# ==========================================
# 2. SETUP BOX2D (MODIFICATO - FetchContent)
# ==========================================
# Questo blocco scarica i sorgenti e li compila con MinGW
# risolvendo l'errore del file .lib incompatibile.
include(FetchContent)

message(STATUS "Downloading and compiling Box2D v3.1.1...")
FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG        v3.1.1
)

# ==========================================
# 3. SETUP SDL3 and SDL3_image (FetchContent)
# ==========================================
# This downloads and builds SDL3 and its image library from source.
message(STATUS "Downloading and compiling SDL3...")
FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.28 # Or a specific release tag like "release-3.1.0"
)

message(STATUS "Downloading and compiling SDL3_image...")
FetchContent_Declare(
    sdl3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-3.2.4 # Or a specific release tag
)

# Make the content available to the project.
# This will download, configure, and add the targets (e.g., SDL3::SDL3)
FetchContent_MakeAvailable(box2d)
FetchContent_MakeAvailable(sdl3)
FetchContent_MakeAvailable(sdl3_image)

# List all your project's source files here.
# This is the recommended and most reliable way to manage sources.
# When you add a new file to your project, add it to this list.
# Find all .c files recursively in the src directory
file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS 
    "src/*.c"
)

# Create the executable from the source files
add_executable(sdl-prototype ${SOURCE_FILES})

# Link libraries to the target executable
target_link_libraries(sdl-prototype PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    box2d
)

# Include header libraries
target_include_directories(sdl-prototype PRIVATE
    "${PROJECT_SOURCE_DIR}/include/cute_headers"    
    "${PROJECT_SOURCE_DIR}/src/includes"
    "${PROJECT_SOURCE_DIR}/include/cglm"
    "${box2d_SOURCE_DIR}/include"
)

# Set project-wide preprocessor definitions for cglm configuration
# This ensures all source files are compiled with the same coordinate system settings
target_compile_definitions(sdl-prototype PRIVATE
    CGLM_FORCE_LEFT_HANDED
    CGLM_FORCE_DEPTH_ZERO_TO_ONE
)

# This will STILL print the path to "my_project/"
message("PROJECT_SOURCE_DIR in src is: ${PROJECT_SOURCE_DIR}")

# This will correctly print the path to "my_project/src/"
message("CMAKE_CURRENT_SOURCE_DIR in src is: ${CMAKE_CURRENT_SOURCE_DIR}")

# Add a custom command to copy the 'assets' folder after the build is complete
add_custom_command(
    TARGET sdl-prototype POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:sdl-prototype>/assets"
    COMMENT "Copying assets folder to output directory"
)

# --- Automatic DLL Copying for Runtime ---
# This command will copy the required SDL and dependency DLLs to the executable's
# directory after it's built. This is crucial for running the application on Windows.
add_custom_command(
    TARGET sdl-prototype POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>"
            "$<TARGET_FILE_DIR:sdl-prototype>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3_image::SDL3_image>"
            "$<TARGET_FILE_DIR:sdl-prototype>"
    # SDL_image may have its own dependencies (like libpng, libjpeg).
    # We need to copy those as well. This expression gets them.
    COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_RUNTIME_DLLS:SDL3_image::SDL3_image>"
            "$<TARGET_FILE_DIR:sdl-prototype>"
    COMMENT "Copying runtime DLLs to output directory"
)
