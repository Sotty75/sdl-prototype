# Minimum version required for cmake
cmake_minimum_required(VERSION 3.15)

# Project definition: name and language
project(sdl-prototype LANGUAGES C)

# C Version Requried
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SDL3 library path
set(SDL3_DIR "F:\\Coding\\libraries\\SDL3\\cmake")
find_package(SDL3 REQUIRED CONFIG)

# SDL3 library path
set(SDL3_image_DIR "F:\\Coding\\Libraries\\SDL_image\\build")
find_package(SDL3_image REQUIRED CONFIG)


# List all your project's source files here.
# This is the recommended and most reliable way to manage sources.
# When you add a new file to your project, add it to this list.
set(SOURCE_FILES
    src/main.c
    src/common.c
    src/sot_scene.c
    src/sot_actor.c
    src/sot_sprite.c   
    src/sot_texture.c
    src/sot_tilemap.c
    src/sot_collider.c
)

# Create the executable from the source files
add_executable(sdl-prototype ${SOURCE_FILES})

# Link libraries to the target executable
target_link_libraries(sdl-prototype PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    "${PROJECT_SOURCE_DIR}/lib/box2d/box2dd.lib"
)

# Include header libraries
target_include_directories(sdl-prototype PRIVATE
    "${PROJECT_SOURCE_DIR}/include/cute_headers"    
    "${PROJECT_SOURCE_DIR}/src/includes"
    "${PROJECT_SOURCE_DIR}/include/cglm"
    "${PROJECT_SOURCE_DIR}/include/box2d"
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
