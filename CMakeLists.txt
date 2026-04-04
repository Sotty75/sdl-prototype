# Minimum version required for cmake
cmake_minimum_required(VERSION 3.21)

# Project definition: name and language
project(sdl-prototype LANGUAGES C CXX)

# C Version Required
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# ==========================================
# 2. SETUP BOX2D (FetchContent)
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
    GIT_TAG        release-3.2.28
)

message(STATUS "Downloading and compiling SDL3_image...")
FetchContent_Declare(
    sdl3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        release-3.2.4
)

# ==========================================
# 4. SETUP LUA 5.4 (FetchContent)
# ==========================================
# ==========================================
# 5. SETUP cimgui (Dear ImGui C bindings) via FetchContent
# ==========================================
message(STATUS "Downloading cimgui (Dear ImGui C bindings)...")
FetchContent_Declare(
    cimgui
    GIT_REPOSITORY https://github.com/cimgui/cimgui.git
    GIT_TAG        docking_inter
)

message(STATUS "Downloading Lua 5.4.7 source...")
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.7
)

# Make the content available to the project.
# This will download, configure, and add the targets (e.g., SDL3::SDL3)
FetchContent_MakeAvailable(box2d)
FetchContent_MakeAvailable(sdl3)
FetchContent_MakeAvailable(sdl3_image)
FetchContent_MakeAvailable(lua)

# Build cimgui as static library
set(IMGUI_STATIC "yes" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(cimgui)

# Add the SDL3 + SDL3 GPU backend sources to cimgui
target_sources(cimgui PRIVATE
    "${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp"
    "${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdlgpu3.cpp"
    "${cimgui_SOURCE_DIR}/cimgui_impl.cpp"
)
target_include_directories(cimgui PUBLIC
    "${cimgui_SOURCE_DIR}"
    "${cimgui_SOURCE_DIR}/imgui"
    "${cimgui_SOURCE_DIR}/imgui/backends"
)
target_link_libraries(cimgui SDL3::SDL3)

# Build Lua as a static library from source (Lua doesn't have a CMakeLists.txt)
# The GitHub mirror places sources directly in the root directory
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
# Remove lua.c (standalone interpreter) and luac.c (compiler) from the library
list(FILTER LUA_SOURCES EXCLUDE REGEX "/(lua|luac|onelua)\\.c$")
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC "${lua_SOURCE_DIR}")
# Suppress warnings in Lua source
target_compile_options(lua_static PRIVATE -w)

# List all your project's source files here.
# This is the recommended and most reliable way to manage sources.
# When you add a new file to your project, add it to this list.
# Find all .c files recursively in the src directory
file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS 
    "src/*.c"
    "include/cJSON/cJSON.c"
)

# Create the executable from the source files
add_executable(sdl-prototype ${SOURCE_FILES})

# Link libraries to the target executable
target_link_libraries(sdl-prototype PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    box2d
    lua_static
    cimgui
)

# Include header libraries
target_include_directories(sdl-prototype PRIVATE
    "${PROJECT_SOURCE_DIR}/include/cute_headers"    
    "${PROJECT_SOURCE_DIR}/include/cJSON"
    "${PROJECT_SOURCE_DIR}/include/cglm"
    "${PROJECT_SOURCE_DIR}/src/includes"
    "${box2d_SOURCE_DIR}/include"
    "${cimgui_SOURCE_DIR}"
    "${cimgui_SOURCE_DIR}/imgui"
    "${cimgui_SOURCE_DIR}/imgui/backends"
)

# Set project-wide preprocessor definitions for cglm configuration
# This ensures all source files are compiled with the same coordinate system settings
target_compile_definitions(sdl-prototype PRIVATE
    CGLM_FORCE_LEFT_HANDED
    CGLM_FORCE_DEPTH_ZERO_TO_ONE
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
    CIMGUI_USE_SDL3=1
)

# ==========================================
# ASSET SYNC (RUNS EVERY BUILD)
# ==========================================
# We use ${CMAKE_BINARY_DIR} instead of $<TARGET_FILE_DIR:...> to break the cycle.
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Syncing assets folder..."
)

# Make sure the game waits for assets to be copied before running/linking
add_dependencies(sdl-prototype CopyAssets)  

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
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:box2d>"
            "$<TARGET_FILE_DIR:sdl-prototype>"
    # SDL_image may have its own dependencies (like libpng, libjpeg).
    # We need to copy those as well. This expression gets them.
    COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_RUNTIME_DLLS:SDL3_image::SDL3_image>"
            "$<TARGET_FILE_DIR:sdl-prototype>"
    COMMENT "Copying runtime DLLs to output directory"
)
